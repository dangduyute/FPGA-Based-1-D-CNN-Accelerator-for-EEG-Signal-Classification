/*
 *-----------------------------------------------------------------------------
 * Title         : Controller (Pruning Final)
 * Project       : CGRA_ECG
 *-----------------------------------------------------------------------------
 * Modification history :
 * 2024.10.24 : created (dense version)
 * 2024.xx.xx : pruning final
 *              - WRAM: Dual_Port_RAM_3 with 1-cycle latency
 *              - Data path (MUX_Selection, ldm_k_base, PE_Incr, Parity,
 *                Padding_Read_SEL): use k_cur/j_cur directly
 *                ? same cycle as douta arriving at PE (both 1-cycle from RAM)
 *              - Flow control (end_of_n gate, wram_base/offset, y/n count,
 *                next_ctx_flg, BRAM_enb): use _d1 pipeline
 *                ? align with cycle when end_of_n is valid
 *-----------------------------------------------------------------------------
 */

`timescale 1ns/1ns
`include "common.vh"

module Controller
(
    input  wire                                     CLK,
    input  wire                                     RST,

    input  wire                                     start_in,
    input  wire [`CTX_BITS-1:0]                     CTX_in,
    input  wire [`CRAM_ADDR_BITS-1:0]               CTX_Max_addr_in,

    // k and j from Dual_Port_RAM_3 - 1-cycle latency from addr
    input  wire [4:0]                               k_cur_in,
    input  wire [2:0]                               j_cur_in,

    ///*** To the Context RAM ***///
    output  wire [`CRAM_ADDR_BITS-1:0]              CTRL_CRAM_addrb_out,
    output  wire                                    CTRL_CRAM_enb_out,
    output  wire                                    CTRL_CRAM_web_out,

    ///*** To the Weight RAM ***///
    output  wire [`WRAM_ADDR_BITS-1:0]              CTRL_WRAM_addrb_out,
    output  wire                                    CTRL_WRAM_enb_out,
    output  wire                                    CTRL_WRAM_web_out,

    ///*** To the Bias RAM ***///
    output  wire [`BRAM_ADDR_BITS-1:0]              CTRL_BRAM_addrb_out,
    output  wire                                    CTRL_BRAM_enb_out,
    output  wire                                    CTRL_BRAM_web_out,

    ///*** To SGB ***///
    output  wire [`ALU_CFG_BITS-1:0]                CFG_out,
    output  wire [`PE_NUM_BITS-1:0]                 MUX_Selection_out,
    output  wire                                    Stride_out,
    output  wire                                    MP_Padding_out,
    output  wire                                    MP_Padding_2_out,
    output  wire                                    MP_Padding_3_out,

    ///*** To All PEs ***///
    output  wire                                    En_out,
    output  wire                                    layer_done_out,
    output  wire                                    Parity_PE_Selection_out,

    output  wire [`S_LDM_BITS+`LDM_ADDR_BITS-1:0]  CTRL_LDM_addra_out,
    output  wire                                    CTRL_LDM_ena_out,
    output  wire                                    CTRL_LDM_wea_out,

    output  wire [`S_LDM_BITS+`LDM_ADDR_BITS-1:0]  CTRL_LDM_addrb_out,
    output  wire                                    CTRL_LDM_enb_out,
    output  wire                                    CTRL_LDM_web_out,

    output  wire [`D_LDM_BITS+`SA_LDM_BITS-1:0]    CTRL_LDM_Store_out,

    ///*** To specific PEs ***///
    output wire                                     Overarray_out,
    output wire [`PE_NUM-1:0]                       Padding_Read_out,
    output wire [`PE_NUM-1:0]                       CTRL_LDM_addra_Incr_out,

    ///*** To AXI Mapper ***///
    output  wire                                    complete_out,
    output reg  [`Y_BITS-1:0]              y_count_d1_rg,
    output reg  [`N_BITS-1:0]              n_count_d1_rg
);

    // =========================================================
    // Wire signals
    // =========================================================
    wire                            load_ctx_wr;
    wire                            complete_flg_wr;
    wire                            next_ctx_flg_wr;

    wire [`PAD_BITS-1:0]            pad_wr;
    wire [`N_BITS-1:0]              n_wr;
    wire [`Y_BITS-1:0]              y_wr;
    wire [`K_BITS-1:0]              k_wr;
    wire [`J_BITS-1:0]              j_wr;
    wire [`STRIDE_BITS-1:0]         stride_wr;
    wire [`S_LDM_BITS-1:0]          source_LDM_wr;
    wire [`D_LDM_BITS-1:0]          destination_LDM_wr;
    wire [`SA_LDM_BITS-1:0]         starting_address_LDM_wr;
    wire [`ALU_CFG_BITS-1:0]        CFG_wr;
    wire [`PE_NUM_BITS-1:0]         Padding_Read_SEL_wr;
    wire [`LDM_ADDR_BITS-1:0]       CTRL_LDM_addra_inc_wr;
    wire [`PE_NUM_BITS-1:0]         MUX_Selection_wr;
    wire [`PE_NUM_BITS-1:0]         MUX_Selection_2_wr;
    wire [`PE_NUM_BITS-1:0]         PE_Selection_wr;
    wire [`PE_NUM_BITS-1:0]         PE_Incr_wr;
    wire [`CTX_BITS-1:0]            CTX_wr;
    wire                            conv_en_wr;
    wire [`LDM_ADDR_BITS-1:0]       CTRL_LDM_addrb_wr;

    // ----------------------------------------------------------
    // Pruning wires
    // k_cur/j_cur: valid at cycle N+1 (1-cycle RAM latency)
    // end_of_n   : sentinel detected, valid at cycle N+1
    // ldm_k_base : from k_cur, valid at N+1 - same as douta to PE
    // ----------------------------------------------------------
    wire [4:0]                      k_cur;
    wire [2:0]                      j_cur;
    wire [`LDM_ADDR_BITS-1:0]       ldm_k_base;
    wire                            end_of_n;

    // =========================================================
    // Register signals
    // =========================================================
    reg  [1:0]                      STATE_rg;
    reg  [`CRAM_ADDR_BITS-1:0]      CTRL_CRAM_addrb_rg;

    // WRAM address (pruning)
    reg  [`WRAM_ADDR_BITS-1:0]      wram_base_rg;
    reg  [`WRAM_ADDR_BITS-1:0]      wram_offset_rg;

    reg  [`BRAM_ADDR_BITS-1:0]      CTRL_BRAM_addra_rg;
    reg  [`N_BITS-1:0]              n_count_rg;
    reg  [`Y_BITS-1:0]              y_count_rg;

    reg                             next_ctx_flg_1_rg;
    reg                             next_ctx_flg_2_rg;
    reg                             next_ctx_flg_3_rg;
    reg                             next_ctx_flg_4_rg;

    reg  [`PE_NUM-1:0]              Padding_Read_rg;
    reg  [`LDM_ADDR_BITS-1:0]       CTRL_LDM_addra_rg;
    reg  [`LDM_ADDR_BITS-1:0]       CTRL_LDM_y_count_rg;
    reg                             MP_Padding_2_rg;
    reg                             MP_Padding_3_rg;
    reg                             Overarray_rg, Overarray_2_rg;
    reg  [`PE_NUM-1:0]              CTRL_LDM_addra_Incr_rg;

    // ----------------------------------------------------------
    // 1-cycle pipeline - for FLOW CONTROL only
    // Aligns conv_en/y_count/n_count/wram_offset with end_of_n
    // (end_of_n is valid at N+1, these signals are at N ? need _d1)
    //
    // NOT used for data path signals (MUX_Selection, ldm_k_base,
    // PE_Incr, Parity, Padding_Read_SEL) - those use k_cur/j_cur
    // directly because douta and k/j arrive at PE at same cycle N+1
    // ----------------------------------------------------------
    reg                             conv_en_d1_rg;
       reg end_of_n_d1_rg;
    reg  [`WRAM_ADDR_BITS-1:0]      wram_offset_d1_rg;
    wire end_of_n_next;
    assign end_of_n_next = (wram_offset_rg == j_wr);
    localparam IDLE     = 0;
    localparam LOAD_CTX = 1;
    localparam EXEC     = 2;
    reg wram_gap_rg;
    // =========================================================
    // Pruning combinational
    // =========================================================
    assign k_cur      = k_cur_in;
    assign j_cur      = j_cur_in;

    // end_of_n: sentinel (k==k_wr && j==j_wr), valid at cycle N+1
    assign end_of_n   = (k_cur == k_wr) && (j_cur == j_wr);

    // ldm_k_base: valid at N+1, same cycle as douta arriving at PE
    assign ldm_k_base = k_cur * CTRL_LDM_addra_inc_wr;

    // =========================================================
    // CTX decoder
    // =========================================================
    assign CTX_wr               = (STATE_rg == EXEC) ? CTX_in : 0;
    assign En_out               = (STATE_rg == EXEC) ? 1'b1 : 0;
    assign load_ctx_wr          = (STATE_rg == LOAD_CTX) ? 1'b1 : 1'b0;

    assign CTRL_CRAM_addrb_out  = CTRL_CRAM_addrb_rg;
    assign CTRL_CRAM_enb_out    = load_ctx_wr;
    assign CTRL_CRAM_web_out    = 0;

    assign pad_wr               = CTX_wr[`CTX_BITS-1:`CTX_BITS-`PAD_BITS];
    assign n_wr                 = CTX_wr[`CTX_BITS-`PAD_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS];
    assign y_wr                 = CTX_wr[`CTX_BITS-`PAD_BITS-`N_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS];
    assign k_wr                 = CTX_wr[`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`K_BITS];
    assign j_wr                 = CTX_wr[`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`K_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`K_BITS-`J_BITS];
    assign CFG_wr               = CTX_wr[`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`J_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`K_BITS-`J_BITS-`ALU_CFG_BITS];
    assign stride_wr            = CTX_wr[`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`J_BITS-`ALU_CFG_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`K_BITS-`J_BITS-`ALU_CFG_BITS-`STRIDE_BITS];
    assign source_LDM_wr        = CTX_wr[`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`J_BITS-`ALU_CFG_BITS-`STRIDE_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`K_BITS-`J_BITS-`ALU_CFG_BITS-`STRIDE_BITS-`S_LDM_BITS];
    assign destination_LDM_wr   = CTX_wr[`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`J_BITS-`ALU_CFG_BITS-`STRIDE_BITS-`S_LDM_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`K_BITS-`J_BITS-`ALU_CFG_BITS-`STRIDE_BITS-`S_LDM_BITS-`D_LDM_BITS];
    assign starting_address_LDM_wr = CTX_wr[`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`J_BITS-`ALU_CFG_BITS-`STRIDE_BITS-`S_LDM_BITS-`D_LDM_BITS-1:`CTX_BITS-`PAD_BITS-`N_BITS-`Y_BITS-`K_BITS-`J_BITS-`ALU_CFG_BITS-`STRIDE_BITS-`S_LDM_BITS-`D_LDM_BITS-`SA_LDM_BITS];

    assign conv_en_wr           = (CFG_wr[`ALU_CFG_BITS-2:0] == `EXE_MAC) ? 1'b1 : 0;

    // =========================================================
    // WRAM - addr sent at cycle N, output valid at N+1
    // =========================================================
    assign CTRL_WRAM_addrb_out  = wram_base_rg + wram_offset_rg;
    assign CTRL_WRAM_enb_out    = (STATE_rg == EXEC) ? conv_en_wr : 1'b0;
    assign CTRL_WRAM_web_out    = 0;
//    assign CTRL_WRAM_enb_out = (STATE_rg == EXEC)
//    ? (conv_en_wr & ~wram_gap_rg)
//    : 1'b0;

//    assign CTRL_WRAM_enb_out = (STATE_rg == EXEC)
//    ? (conv_en_wr & ~(end_of_n & conv_en_d1_rg))
//    : 1'b0;
    // =========================================================
    // BRAM - use conv_en_d1 + end_of_n (both valid at N+1)
    // =========================================================
    assign CTRL_BRAM_addrb_out  = CTRL_BRAM_addra_rg;
    assign CTRL_BRAM_enb_out    = ((STATE_rg == EXEC) & end_of_n)
                                ? conv_en_d1_rg : 1'b0;
    assign CTRL_BRAM_web_out    = 0;

    // =========================================================
    // SGB - DATA PATH: use j_cur directly (valid at N+1 = same as douta)
    // =========================================================
    assign MUX_Selection_2_wr   = MUX_Selection_wr
                                + ((j_cur + (stride_wr & pad_wr[0])) >> stride_wr);
    assign MUX_Selection_out    = (MUX_Selection_2_wr >= `PE_NUM)
                                ? MUX_Selection_2_wr - `PE_NUM
                                : MUX_Selection_wr + (j_cur >> (stride_wr & ~pad_wr[0]));
    assign Stride_out           = stride_wr;
    assign MP_Padding_out       = pad_wr[0];
    assign MP_Padding_2_out     = (pad_wr[0]) ? MP_Padding_2_rg : 0;
    assign MP_Padding_3_out     = (~pad_wr[0]) ? MP_Padding_3_rg : 0;

    // =========================================================
    // PEs
    // =========================================================
    assign CFG_out              = CFG_wr;
    assign layer_done_out       = next_ctx_flg_wr | complete_flg_wr;

    assign CTRL_LDM_addra_inc_wr =
        ((stride_wr == 0) & (y_wr == 7)) ? 8  :
        ((stride_wr == 1) & (y_wr == 7)) ? 16 :
        ((stride_wr == 0) & (y_wr == 3)) ? 4  :
        ((stride_wr == 1) & (y_wr == 3)) ? 8  :
        ((stride_wr == 0) & (y_wr == 1)) ? 2  :
        ((stride_wr == 1) & (y_wr == 1)) ? 4  : 1;

    assign MUX_Selection_wr     = (pad_wr == 0) ? 0
                                : (stride_wr & ((pad_wr == 2) | (pad_wr == 1))) ? 19
                                : `PE_NUM - pad_wr;

    // DATA PATH: ldm_k_base t? k_cur (valid N+1 = same as douta)
    assign CTRL_LDM_addra_out   = {source_LDM_wr, ldm_k_base + CTRL_LDM_y_count_rg};
    assign CTRL_LDM_ena_out     = (STATE_rg == EXEC) ? 1'b1 : 1'b0;
    assign CTRL_LDM_wea_out     = 0;

    assign CTRL_LDM_addrb_wr    = ldm_k_base + CTRL_LDM_y_count_rg + 1;
    assign CTRL_LDM_addrb_out   = {source_LDM_wr, CTRL_LDM_addrb_wr};
    assign CTRL_LDM_enb_out     = (STATE_rg == EXEC) ? 1'b1 : 1'b0;
    assign CTRL_LDM_web_out     = 0;

    assign CTRL_LDM_Store_out   = {destination_LDM_wr, starting_address_LDM_wr};

    assign Overarray_out        = Overarray_2_rg;
    assign PE_Selection_wr      = (pad_wr == 0) ? 0 : `PE_NUM - pad_wr;

    // DATA PATH: PE_Incr t? j_cur (valid N+1)
    assign PE_Incr_wr           = ((PE_Selection_wr + j_cur) >= `PE_NUM)
                                ? PE_Selection_wr + j_cur - `PE_NUM
                                : PE_Selection_wr + j_cur;
    assign CTRL_LDM_addra_Incr_out = CTRL_LDM_addra_Incr_rg;

    // DATA PATH: Padding_Read_SEL dùng j_cur (valid N+1)
    // Dùng y_count_d1_rg ?? align v?i j_cur t?i N+1
    assign Padding_Read_SEL_wr  =
        ((y_count_d1_rg == 0) & (j_cur < pad_wr))
            ? `PE_NUM - pad_wr + j_cur
        : ((pad_wr != 0) & (y_count_d1_rg == y_wr)
            & (j_cur >= (j_wr - pad_wr)))
            ? j_cur - pad_wr - 1
        : 31;
    assign Padding_Read_out     = Padding_Read_rg;

    assign next_ctx_flg_wr      = next_ctx_flg_4_rg;
    assign complete_flg_wr      = next_ctx_flg_4_rg
                                & (CTRL_CRAM_addrb_rg > CTX_Max_addr_in);
    assign complete_out         = complete_flg_wr;

    // DATA PATH: Parity t? j_cur (valid N+1)
    assign Parity_PE_Selection_out = (pad_wr[0] == 1) ? ~j_cur[0] : j_cur[0];

    // =========================================================
    // State Machine
    // =========================================================
    always @(posedge CLK or negedge RST) begin
    if (~RST) end_of_n_d1_rg <= 0;
    else      end_of_n_d1_rg <= end_of_n & conv_en_d1_rg;
end

    always @(posedge CLK or negedge RST) begin
        if (~RST) begin
            STATE_rg <= IDLE;
        end
        else begin
            if ((STATE_rg == IDLE) & start_in)
                STATE_rg <= LOAD_CTX;
            else if (STATE_rg == LOAD_CTX)
                STATE_rg <= EXEC;
            else if ((STATE_rg == EXEC) & complete_flg_wr)
                STATE_rg <= IDLE;
            else if ((STATE_rg == EXEC) & next_ctx_flg_wr)
                STATE_rg <= LOAD_CTX;
            else
                STATE_rg <= STATE_rg;
        end
    end

    // =========================================================
    // 1-cycle pipeline - FLOW CONTROL only
    // conv_en/y_count/n_count/wram_offset are at cycle N
    // end_of_n is at cycle N+1
    // _d1 versions align these control signals with end_of_n
    // =========================================================
    always @(posedge CLK or negedge RST) begin
        if (~RST) begin
            conv_en_d1_rg     <= 0;
            y_count_d1_rg     <= 0;
            wram_offset_d1_rg <= 0;
            n_count_d1_rg     <= 0;
        end
        else begin
            if (STATE_rg == IDLE || STATE_rg == LOAD_CTX) begin
                conv_en_d1_rg     <= 0;
                y_count_d1_rg     <= 0;
                wram_offset_d1_rg <= 0;
                n_count_d1_rg     <= 0;
            end
            else begin
                conv_en_d1_rg     <= conv_en_wr;
                y_count_d1_rg     <= y_count_rg;
                wram_offset_d1_rg <= wram_offset_rg;
                n_count_d1_rg     <= n_count_rg;
            end
        end
    end
//always @(posedge CLK or negedge RST) begin
//    if (~RST) begin
//        conv_en_d1_rg     <= 0;
//        y_count_d1_rg     <= 0;
//        wram_offset_d1_rg <= 0;
//        n_count_d1_rg     <= 0;
//        wram_gap_rg       <= 0;
//    end
//    else begin
//        if (STATE_rg == IDLE || STATE_rg == LOAD_CTX) begin
//            conv_en_d1_rg     <= 0;
//            y_count_d1_rg     <= 0;
//            wram_offset_d1_rg <= 0;
//            n_count_d1_rg     <= 0;
//            wram_gap_rg       <= 0;
//        end
//        else begin
//            // Gap = 1 khi sentinel v?a ???c detect
//            // Clear t? ??ng cycle sau
//            wram_gap_rg       <= end_of_n & conv_en_d1_rg;
            
//            // conv_en_d1 b? clear trong gap cycle
//            // ? ng?n main counter và BRAM enb trigger sai
//            conv_en_d1_rg     <= conv_en_wr & ~wram_gap_rg;
//            y_count_d1_rg     <= y_count_rg;
//            wram_offset_d1_rg <= wram_offset_rg;
//            n_count_d1_rg     <= n_count_rg;
//        end
//    end
//end
    // =========================================================
    // WRAM address control - FLOW CONTROL uses _d1
    // wram_base/offset updated at cycle N+1 using _d1 signals
    // so next addr at cycle N+2 is correct
    // =========================================================
    always @(posedge CLK or negedge RST) begin
        if (~RST) begin
            wram_base_rg   <= 0;
            wram_offset_rg <= 0;
        end
        else begin
            if (STATE_rg == IDLE || STATE_rg == LOAD_CTX) begin
                wram_base_rg   <= 0;
                wram_offset_rg <= 0;
            end
            else begin // EXEC
                if (conv_en_wr) begin
                    if (end_of_n) begin
                        // Sentinel detected at N+1
                        wram_offset_rg <= 0;
                        if (y_count_d1_rg == y_wr)
                            // All y done ? advance base past NNZ + sentinel
                            wram_base_rg <= wram_base_rg
                                          + wram_offset_d1_rg + 1;
                        else
                            // More y ? keep base, replay NNZ block
                            wram_base_rg <= wram_base_rg;
                    end
                    else begin
                        wram_offset_rg <= wram_offset_rg + 1;
                        wram_base_rg   <= wram_base_rg;
                    end
                end
            end
        end
    end

    // =========================================================
    // Main counter block - FLOW CONTROL uses _d1
    // end_of_n valid at N+1, use _d1 signals to match
    // =========================================================
    always @(posedge CLK or negedge RST) begin
        if (~RST) begin
            n_count_rg             <= 0;
            y_count_rg             <= 0;
            next_ctx_flg_1_rg      <= 0;
            next_ctx_flg_2_rg      <= 0;
            next_ctx_flg_3_rg      <= 0;
            next_ctx_flg_4_rg      <= 0;
            CTRL_LDM_addra_rg      <= 0;
            CTRL_LDM_addra_Incr_rg <= 0;
            CTRL_LDM_y_count_rg    <= 0;
            CTRL_BRAM_addra_rg     <= 0;
            MP_Padding_2_rg        <= 0;
            MP_Padding_3_rg        <= 0;
            Overarray_rg           <= 0;
            Overarray_2_rg         <= 0;
        end
        else begin
            Overarray_2_rg <= Overarray_rg;

            if (STATE_rg == IDLE) begin
                n_count_rg             <= 0;
                y_count_rg             <= 0;
                CTRL_LDM_addra_rg      <= 0;
                CTRL_LDM_addra_Incr_rg <= 0;
                CTRL_LDM_y_count_rg    <= 0;
                next_ctx_flg_1_rg      <= 0;
                next_ctx_flg_2_rg      <= 0;
                next_ctx_flg_3_rg      <= 0;
                next_ctx_flg_4_rg      <= 0;
                MP_Padding_2_rg        <= 0;
                MP_Padding_3_rg        <= 0;
                CTRL_BRAM_addra_rg     <= 0;
                Overarray_rg           <= 0;
            end
            else if (STATE_rg == LOAD_CTX) begin
                n_count_rg             <= 0;
                y_count_rg             <= 0;
                CTRL_LDM_addra_rg      <= 0;
                CTRL_LDM_addra_Incr_rg <= 0;
                CTRL_LDM_y_count_rg    <= 0;
                next_ctx_flg_1_rg      <= 0;
                next_ctx_flg_2_rg      <= 0;
                next_ctx_flg_3_rg      <= 0;
                next_ctx_flg_4_rg      <= 0;
                MP_Padding_2_rg        <= 0;
                MP_Padding_3_rg        <= 0;
                Overarray_rg           <= 0;
            end
            else begin // EXEC

                // FLOW CONTROL: end_of_n & conv_en_d1_rg (both at N+1)
                if (end_of_n & conv_en_d1_rg) begin
                    // Sentinel detected - end of NNZ for current y
                    CTRL_LDM_addra_Incr_rg <= 0;
                    CTRL_LDM_addra_rg      <= 0;

                    if (y_count_d1_rg == y_wr) begin
                        // End of all y for current n
                        y_count_rg          <= 0;
                        MP_Padding_2_rg     <= 1;
                        CTRL_LDM_y_count_rg <= 0;
                        Overarray_rg        <= 1;

                        if (n_count_d1_rg == n_wr) begin
                            n_count_rg         <= 0;
                            CTRL_BRAM_addra_rg <= CTRL_BRAM_addra_rg + 1;
                            MP_Padding_3_rg    <= 1;
                        end
                        else begin
                            n_count_rg         <= n_count_d1_rg + 1;
                            CTRL_BRAM_addra_rg <= CTRL_BRAM_addra_rg + 1;
                            MP_Padding_3_rg    <= 0;
                        end
                    end
                    else begin
                        // More y remaining for current n
                        y_count_rg          <= y_count_d1_rg + 1;
                        CTRL_LDM_y_count_rg <= CTRL_LDM_y_count_rg
                                             + 1 + stride_wr;
                        MP_Padding_2_rg     <= 0;
                        Overarray_rg        <= 0;
                    end
                end
                else if (~end_of_n & conv_en_d1_rg) begin
                    // Still within NNZ of current y
                    // PE_Incr_wr is from j_cur (data path, valid N+1)
                    CTRL_LDM_addra_Incr_rg[PE_Incr_wr] <= 1'b1;
                    Overarray_rg <= 0;
                end

                // next_ctx_flg pipeline - 4-cycle delay (unchanged)
                next_ctx_flg_2_rg <= next_ctx_flg_1_rg;
                next_ctx_flg_3_rg <= next_ctx_flg_2_rg;
                next_ctx_flg_4_rg <= next_ctx_flg_3_rg;

                // FLOW CONTROL: all _d1 at N+1
                if ((n_count_d1_rg == n_wr) & (y_count_d1_rg == y_wr)
                    & end_of_n & conv_en_d1_rg)
                    next_ctx_flg_1_rg <= 1;
                else
                    next_ctx_flg_1_rg <= 0;
            end
        end
    end

    // =========================================================
    // CRAM address (unchanged)
    // =========================================================
    always @(posedge CLK or negedge RST) begin
        if (~RST) begin
            CTRL_CRAM_addrb_rg <= 0;
        end
        else begin
            if (complete_flg_wr)
                CTRL_CRAM_addrb_rg <= 0;
            else if (load_ctx_wr)
                CTRL_CRAM_addrb_rg <= CTRL_CRAM_addrb_rg + load_ctx_wr;
            else
                CTRL_CRAM_addrb_rg <= CTRL_CRAM_addrb_rg;
        end
    end

    // =========================================================
    // Padding Read (unchanged)
    // =========================================================
    always @(*) begin
        case (Padding_Read_SEL_wr)
            5'd0:    Padding_Read_rg = 20'b0000_0000_0000_0000_0001;
            5'd1:    Padding_Read_rg = 20'b0000_0000_0000_0000_0011;
            5'd2:    Padding_Read_rg = 20'b0000_0000_0000_0000_0111;
            5'd3:    Padding_Read_rg = 20'b0000_0000_0000_0000_1000;
            5'd17:   Padding_Read_rg = 20'b1110_0000_0000_0000_0000;
            5'd18:   Padding_Read_rg = 20'b1100_0000_0000_0000_0000;
            5'd19:   Padding_Read_rg = 20'b1000_0000_0000_0000_0000;
            default: Padding_Read_rg = 20'b0000_0000_0000_0000_0000;
        endcase
    end

endmodule