#include <cmath>  // Include cmath for the round function
#include <cstdio>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

// Define bit lengths as macros
#define CTX_BITS       32  // Total bits in the final result
#define PAD_BITS       2
#define N_BITS         5
#define Y_BITS         3
#define K_BITS         5
#define J_BITS         3
#define ALU_CFG_BITS   3
#define STRIDE_BITS    1
#define S_LDM_BITS     2
#define D_LDM_BITS     2
#define SA_LDM_BITS    6

#define NUM_VALUES 1280
#define FRACTIONAL_BITS 6
#define SCALE_FACTOR (1 << FRACTIONAL_BITS)

int weight_addr = 0;
int bias_addr = 0;
int ctx_addr = 0;

FILE *weight_file = fopen("WRAM_File.txt", "w");
FILE *bias_file = fopen("BRAM_File.txt", "w");
FILE *context_file = fopen("CRAM_File.txt", "w");

// Function to concatenate inputs into a 32-bit integer
uint32_t concatenate_to_32bit(int pad, int n, int y, int k, int j, int alu_cfg, int stride, int s_ldm, int d_ldm, int sa_ldm) {
    uint32_t result = 0;

    // Ensure inputs are within their respective bit limits
    pad     &= (1 << PAD_BITS) - 1;
    n       &= (1 << N_BITS) - 1;
    y       &= (1 << Y_BITS) - 1;
    k       &= (1 << K_BITS) - 1;
    j       &= (1 << J_BITS) - 1;
    alu_cfg &= (1 << ALU_CFG_BITS) - 1;
    stride  &= (1 << STRIDE_BITS) - 1;
    s_ldm   &= (1 << S_LDM_BITS) - 1;
    d_ldm   &= (1 << D_LDM_BITS) - 1;
    sa_ldm  &= (1 << SA_LDM_BITS) - 1;

    // Concatenate the inputs into a single 32-bit integer
    result |= (pad     << (CTX_BITS - PAD_BITS));                                 // Pad bits
    result |= (n       << (CTX_BITS - PAD_BITS - N_BITS));                        // N bits
    result |= (y       << (CTX_BITS - PAD_BITS - N_BITS - Y_BITS));               // Y bits
    result |= (k       << (CTX_BITS - PAD_BITS - N_BITS - Y_BITS - K_BITS));      // K bits
    result |= (j       << (CTX_BITS - PAD_BITS - N_BITS - Y_BITS - K_BITS - J_BITS));  // J bits
    result |= (alu_cfg << (CTX_BITS - PAD_BITS - N_BITS - Y_BITS - K_BITS - J_BITS - ALU_CFG_BITS));  // ALU CFG bits
    result |= (stride  << (CTX_BITS - PAD_BITS - N_BITS - Y_BITS - K_BITS - J_BITS - ALU_CFG_BITS - STRIDE_BITS));  // Stride bits
    result |= (s_ldm   << (CTX_BITS - PAD_BITS - N_BITS - Y_BITS - K_BITS - J_BITS - ALU_CFG_BITS - STRIDE_BITS - S_LDM_BITS));  // S_LDM bits
    result |= (d_ldm   << (CTX_BITS - PAD_BITS - N_BITS - Y_BITS - K_BITS - J_BITS - ALU_CFG_BITS - STRIDE_BITS - S_LDM_BITS - D_LDM_BITS));  // D_LDM bit
    result |= (sa_ldm  << (CTX_BITS - PAD_BITS - N_BITS - Y_BITS - K_BITS - J_BITS - ALU_CFG_BITS - STRIDE_BITS - S_LDM_BITS - D_LDM_BITS - SA_LDM_BITS));  // SA_LDM bits

    return result;
}

// Define the scale factor for 6 fractional bits
#define FRACTIONAL_BITS 6
#define SCALE_FACTOR (1 << FRACTIONAL_BITS)

// Function to convert float to 16-bit fixed-point (1 sign bit, 9 integer bits, 6 fractional bits)
int16_t FX_convert(float input) {
    // Multiply the input by the scale factor
    float scaled_value = input * SCALE_FACTOR;

    // Proper rounding towards zero for negative numbers
    int16_t fixed_value;
    if (scaled_value >= 0) {
        fixed_value = (int16_t)(scaled_value + 0.5f);  // Round up for positive values
    } else {
        fixed_value = (int16_t)(scaled_value - 0.5f);  // Round down for negative values
    }

    // Handle overflow and underflow cases explicitly within 16-bit range
    if (fixed_value > 32767) {
        fixed_value = 32767;
    } else if (fixed_value < -32768) {
        fixed_value = -32768;
    }

    // Return the fixed-point value
    return fixed_value;
}

// Function to write data to file in "address_data" format
void write_weight_to_file(float data[], int length) {
    
    for (int i = 0; i < length; i++) {
        int fixed_point_value = FX_convert(data[i]);
        // Ensure 16-bit representation of the address and data
        // Mask to 16-bit address
        int data_16bit = fixed_point_value & 0xFFFF; // Mask to 16-bit data
        
        fprintf(weight_file, "%04x_%04x\n", weight_addr, data_16bit);
		weight_addr ++; 
    }
}
// void write_weight_to_file(float data[], int length) {
    
//     for (int i = 0; i < length; i++) {
//         int fixed_point_value = FX_convert(data[i]);
//         // Ensure 16-bit representation of the address and data
//         // Mask to 16-bit address
//         int data_16bit = fixed_point_value & 0xFFFF; // Mask to 16-bit data
        
//         fprintf(weight_file, "%04x\n", data_16bit);
// 		weight_addr ++; 
//     }
// }
// void write_weight_to_file_2(float data[], int z, int k, int j) {
    
// 	int q = 1;
//     for (int i = 0; i < z; i++){
		
//         for (int n = 0; n < k; n++){
//             for (int m = 0; m < j; m++){

//                 int idx = i*(k*j) + n*j + m;

//                 // prune: bỏ giá trị nhỏ
//                 if (fabs(data[idx]) > 0.046875){

//                     int fixed_point_value = FX_convert(data[idx]);
//                     int data_16bit = fixed_point_value & 0xFFFF;

//                     int addr = (n << 3) | m;  // k shift 3 bit (vì j=7 ~ 3bit)

//                     int data_24bit = (data_16bit << 8) | addr;

//                     fprintf(weight_prun_file, "%06x_%0d\n", data_24bit,q);
//                     q++;
//                 }
//             }
//         }	
//     }
// }
// void write_weight_to_file_2(float data[], int z, int k, int j) {
    
//     int q = 1;

//     for (int i = 0; i < z; i++){
//         for (int n = 0; n < k; n++){
//             for (int m = 0; m < j; m++){

//                 int idx = i*(k*j) + n*j + m;

//                 // ✅ giữ phần tử đầu và cuối
//                 int is_edge = (m == 0) || (m == j-1);

//                 // prune nhưng giữ edge
//                 if (is_edge || fabs(data[idx]) > 0.046875){

//                     int fixed_point_value = FX_convert(data[idx]);
//                     int data_16bit = fixed_point_value & 0xFFFF;

//                     int addr = (n << 3) | m;

//                     int data_24bit = (data_16bit << 8) | addr;

//                     fprintf(weight_prun_file, "%06x\n", data_24bit);
//                     q++;
//                 }
//             }
//         }	
//     }
// }
void write_bias_to_file(float data[], int length) {
   
    for (int i = 0; i < length; i++) {
        int fixed_point_value = FX_convert(data[i]);
        // Ensure 16-bit representation of the address and data
        int data_16bit = fixed_point_value & 0xFFFF; // Mask to 16-bit data
        
        fprintf(bias_file, "%04x_%04x\n", bias_addr, data_16bit);
		bias_addr++;
    }
	
}
// void write_bias_to_file(float data[], int length) {
   
//     for (int i = 0; i < length; i++) {
//         int fixed_point_value = FX_convert(data[i]);
//         // Ensure 16-bit representation of the address and data
//         int data_16bit = fixed_point_value & 0xFFFF; // Mask to 16-bit data
        
//         fprintf(bias_file, "%04x\n", data_16bit);
// 		bias_addr++;
//     }
	
// }
// Function to write data to file in "address_data" format
void write_context_to_file(uint32_t data[], int length) {
    for (int i = 0; i < length; i++) {        
        fprintf(context_file, "%04x_%08x\n", ctx_addr, data[i]);
		ctx_addr++;
    }
	
}
// void write_context_to_file(uint32_t data[], int length) {
//     for (int i = 0; i < length; i++) {        
//         fprintf(context_file, "%08x\n", data[i]);
// 		ctx_addr++;
//     }
	
// }
// Function to write data to file in "address_data" format
void write_to_file(const char* filename, float data[], int length) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Unable to open file %s for writing.\n", filename);
        return;
    }

    for (int i = 0; i < length; i++) {
        int fixed_point_value = FX_convert(data[i]);
        // Ensure 16-bit representation of the address and data
	
        int address = (i) & 0xFFFF; // Mask to 16-bit address
		// printf("address = %d\n",address);
        int data_16bit = fixed_point_value & 0xFFFF; // Mask to 16-bit data
        
        fprintf(file, "%04x_%04x\n", address, data_16bit);
    }

    fclose(file);
}

// Function to write data to file in "address_data" format
void write_to_file2(const char* filename, float data[], int length) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Unable to open file %s for writing.\n", filename);
        return;
    }

    for (int i = 0; i < length; i++) {
        int fixed_point_value = FX_convert(data[i]);
        // Ensure 16-bit representation of the address and data
		int a = i/20;
        int address = (i + a*12) & 0xFFFF; // Mask to 16-bit address
		// printf("address = %d\n",address);
        int data_16bit = fixed_point_value & 0xFFFF; // Mask to 16-bit data
        
        fprintf(file, "%04x_%04x\n", address, data_16bit);
    }

    fclose(file);
}



// Helper function to round a value to three decimal places
float fixedpoint_converter(float value) {
	if(value >= 512){
		printf("Value is larger than 512 = %f\n", value);
		value = value - 512;	
	}
    float scalingFactor = 64.0f; // 2^5
    return round(value * scalingFactor) / scalingFactor; 
}
void Padding_Conv1D_0(float input_Pad_Conv[320], float output_Pad_Conv[325]){
	write_to_file2("LDM_File.txt", input_Pad_Conv, 340);
	loop_for_3_channel_pad_0:
	for (int c = 0; c < 1; c++){
		loop_for_channel_pad_0:
		for (int n = 0; n < 325; n++){
			if (n < 2 || n >= 322) output_Pad_Conv[325 * c + n]=0; else output_Pad_Conv[325 * c + n] = input_Pad_Conv[320 * c + n - 2];
		}
	}
	write_to_file("input0.txt", output_Pad_Conv, 325);
}
void Conv1D_0(float Input_Conv[325],float Output_Conv[640], float bias[4], float kernel[28]){
	loop_for_channel_0:
	
	int stride = 2;
	for (int i = 0; i < 325; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 4; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 28; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 4; n++){
		loop_for_ap_0:
		for (int y = 0; y < 160; y++){
			float s = 0;
			loop_for_fc_0:
			for (int k = 0; k < 1; k++){
				loop_for_fa_0:
				for (int j = 0; j < 7; j++){
					s=s+(kernel[1*7*n+7*k+j])*(Input_Conv[325*k+j+y*stride]);}
			}
			if ((s+bias[n])<0) Output_Conv[160*n+y]=0; else Output_Conv[160*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 640; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	write_to_file("output0.txt", Output_Conv, 640);
	write_bias_to_file(bias, 4);
    write_weight_to_file(kernel, 28);
	uint32_t pad_ctx = 2, n_ctx = 3, y_ctx = 7, k_ctx = 0, j_ctx = 6, alu_cfg_ctx =5, stride_ctx = 1,s_ldm_ctx = 0, d_ldm_ctx = 1, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
void Padding_Conv1D_1(float input_Pad_Conv[640], float output_Pad_Conv[656]){
	loop_for_3_channel_pad_1:
	for (int c = 0; c < 4; c++){
		loop_for_channel_pad_1:
		for (int n = 0; n < 164; n++){
			if (n < 2 || n >= 162) output_Pad_Conv[164 * c + n]=0; else output_Pad_Conv[164 * c + n] = input_Pad_Conv[160 * c + n - 2];
		}
	}
}
void Conv1D_1(float Input_Conv[656],float Output_Conv[1280], float bias[8], float kernel[160]){
	loop_for_channel_1:
	int stride = 1;
	for (int i = 0; i < 656; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 8; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 160; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 8; n++){
		loop_for_ap_1:
		for (int y = 0; y < 160; y++){
			float s = 0;
			loop_for_fc_1:
			for (int k = 0; k < 4; k++){
				loop_for_fa_1:
				for (int j = 0; j < 5; j++){
					s=s+(kernel[4*5*n+5*k+j])*(Input_Conv[164*k+j+y*stride]);}
			}
			Output_Conv[160*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	
	write_bias_to_file(bias, 8);
    write_weight_to_file(kernel, 160);
	uint32_t pad_ctx = 2, n_ctx = 7, y_ctx = 7, k_ctx = 3, j_ctx = 4, alu_cfg_ctx = 5, stride_ctx = 0,s_ldm_ctx = 1, d_ldm_ctx = 0, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
 void Activation0(float Input_Activation[1280], float Output_Activation[1280]){
	for (int i = 0; i < 1280; i++){
		if(Input_Activation[i] > 0){
			Output_Activation[i] = Input_Activation[i];
		}else
		{
			Output_Activation[i] = 0;
		}
	}
	write_to_file("output1.txt", Output_Activation, 1280);
}
void Padding_Conv1D_2(float input_Pad_Conv[1280], float output_Pad_Conv[1312]){
	loop_for_3_channel_pad_2:
	for (int c = 0; c < 8; c++){
		loop_for_channel_pad_2:
		for (int n = 0; n < 164; n++){
			if (n < 2 || n >= 162) output_Pad_Conv[164 * c + n]=0; else output_Pad_Conv[164 * c + n] = input_Pad_Conv[160 * c + n - 2];
		}
	}
}
void Conv1D_2(float Input_Conv[1312],float Output_Conv[1280], float bias[8], float kernel[320]){
	loop_for_channel_2:
	int stride = 1;
	for (int i = 0; i < 1312; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 8; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 320; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 8; n++){
		loop_for_ap_2:
		for (int y = 0; y < 160; y++){
			float s = 0;
			loop_for_fc_2:
			for (int k = 0; k < 8; k++){
				loop_for_fa_2:
				for (int j = 0; j < 5; j++){
					s=s+(kernel[8*5*n+5*k+j])*(Input_Conv[164*k+j+y*stride]);}
			}
			Output_Conv[160*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	write_to_file("output2.txt", Output_Conv, 1280);
	write_bias_to_file(bias, 8);
    write_weight_to_file(kernel, 320);
	uint32_t pad_ctx = 2, n_ctx = 7, y_ctx = 7, k_ctx = 7, j_ctx = 4, alu_cfg_ctx = 1, stride_ctx = 0,s_ldm_ctx = 0, d_ldm_ctx = 2, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
void Padding_Conv1D_3(float input_Pad_Conv[640], float output_Pad_Conv[640]){
	loop_for_3_channel_pad_3:
	for (int c = 0; c < 4; c++){
		loop_for_channel_pad_3:
		for (int n = 0; n < 160; n++){
			if (n < 0 || n >= 160) output_Pad_Conv[160 * c + n]=0; else output_Pad_Conv[160 * c + n] = input_Pad_Conv[160 * c + n - 0];
		}
	}
}
void Conv1D_3(float Input_Conv[640],float Output_Conv[1280], float bias[8], float kernel[32]){
	loop_for_channel_3:
	int stride = 1;
	for (int i = 0; i < 640; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 8; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 32; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 8; n++){
		loop_for_ap_3:
		for (int y = 0; y < 160; y++){
			float s = 0;
			loop_for_fc_3:
			for (int k = 0; k < 4; k++){
				loop_for_fa_3:
				for (int j = 0; j < 1; j++){
					s=s+(kernel[4*1*n+1*k+j])*(Input_Conv[160*k+j+y*stride]);}
			}
			Output_Conv[160*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	write_to_file("output3.txt", Output_Conv, 1280);
	write_bias_to_file(bias, 8);
    write_weight_to_file(kernel, 32);
	uint32_t pad_ctx = 0, n_ctx = 7, y_ctx = 7, k_ctx = 3, j_ctx = 0, alu_cfg_ctx = 1, stride_ctx = 0,s_ldm_ctx = 1, d_ldm_ctx = 3, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
void Add_0(float input_0[1280], float input_1[1280], float output[1280]) {
	for (int i = 0; i < 1280; i++){
		output[i] = input_0[i] + input_1[i];
	}
	uint32_t pad_ctx = 0, n_ctx = 7, y_ctx = 7, k_ctx = 0, j_ctx = 0, alu_cfg_ctx = 6, stride_ctx = 0,s_ldm_ctx = 2, d_ldm_ctx = 1, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
 void Activation1(float Input_Activation[1280], float Output_Activation[1280]){
	for (int i = 0; i < 1280; i++){
		if(Input_Activation[i] > 0){
			Output_Activation[i] = Input_Activation[i];
		}else
		{
			Output_Activation[i] = 0;
		}
	}
	write_to_file("output4.txt", Output_Activation, 1280);
}
void Padding_Conv1D_4(float input_Pad_Conv[1280], float output_Pad_Conv[1304]){
	loop_for_3_channel_pad_4:
	for (int c = 0; c < 8; c++){
		loop_for_channel_pad_4:
		for (int n = 0; n < 163; n++){
			if (n < 1 || n >= 161) output_Pad_Conv[163 * c + n]=0; else output_Pad_Conv[163 * c + n] = input_Pad_Conv[160 * c + n - 1];
		}
	}
}
void Conv1D_4(float Input_Conv[1304],float Output_Conv[1280], float bias[16], float kernel[640]){
	loop_for_channel_4:
	int stride = 2;
	for (int i = 0; i < 1304; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 16; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 640; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 16; n++){
		loop_for_ap_4:
		for (int y = 0; y < 80; y++){
			float s = 0;
			loop_for_fc_4:
			for (int k = 0; k < 8; k++){
				loop_for_fa_4:
				for (int j = 0; j < 5; j++){
					s=s+(kernel[8*5*n+5*k+j])*(Input_Conv[163*k+j+y*stride]);}
			}
			if ((s+bias[n])<0) Output_Conv[80*n+y]=0; else Output_Conv[80*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	write_to_file("output6.txt", Output_Conv, 1280);
	write_bias_to_file(bias, 16);
    write_weight_to_file(kernel, 640);
	uint32_t pad_ctx = 1, n_ctx = 15, y_ctx = 3, k_ctx = 7, j_ctx = 4, alu_cfg_ctx = 5, stride_ctx = 1,s_ldm_ctx = 2, d_ldm_ctx = 3, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
void Padding_Conv1D_5(float input_Pad_Conv[1280], float output_Pad_Conv[1344]){
	loop_for_3_channel_pad_5:
	for (int c = 0; c < 16; c++){
		loop_for_channel_pad_5:
		for (int n = 0; n < 84; n++){
			if (n < 2 || n >= 82) output_Pad_Conv[84 * c + n]=0; else output_Pad_Conv[84 * c + n] = input_Pad_Conv[80 * c + n - 2];
		}
	}
}
void Conv1D_5(float Input_Conv[1344],float Output_Conv[1280], float bias[16], float kernel[1280]){
	loop_for_channel_5:
	int stride = 1;
	for (int i = 0; i < 1344; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 16; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 1280; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 16; n++){
		loop_for_ap_5:
		for (int y = 0; y < 80; y++){
			float s = 0;
			loop_for_fc_5:
			for (int k = 0; k < 16; k++){
				loop_for_fa_5:
				for (int j = 0; j < 5; j++){
					s=s+(kernel[16*5*n+5*k+j])*(Input_Conv[84*k+j+y*stride]);}
			}
			Output_Conv[80*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	
	write_bias_to_file(bias, 16);
    write_weight_to_file(kernel, 1280);
	uint32_t pad_ctx = 2, n_ctx = 15, y_ctx = 3, k_ctx = 15, j_ctx = 4, alu_cfg_ctx = 5, stride_ctx = 0,s_ldm_ctx = 3, d_ldm_ctx = 0, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
 void Activation2(float Input_Activation[1280], float Output_Activation[1280]){
	for (int i = 0; i < 1280; i++){
		if(Input_Activation[i] > 0){
			Output_Activation[i] = Input_Activation[i];
		}else
		{
			Output_Activation[i] = 0;
		}
	}
	write_to_file("output7.txt", Output_Activation, 1280);
}
void Padding_Conv1D_6(float input_Pad_Conv[1280], float output_Pad_Conv[1344]){
	loop_for_3_channel_pad_6:
	for (int c = 0; c < 16; c++){
		loop_for_channel_pad_6:
		for (int n = 0; n < 84; n++){
			if (n < 2 || n >= 82) output_Pad_Conv[84 * c + n]=0; else output_Pad_Conv[84 * c + n] = input_Pad_Conv[80 * c + n - 2];
		}
	}
}
void Conv1D_6(float Input_Conv[1344],float Output_Conv[1280], float bias[16], float kernel[1280]){
	loop_for_channel_6:
	int stride = 1;
	for (int i = 0; i < 1344; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 16; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 1280; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 16; n++){
		loop_for_ap_6:
		for (int y = 0; y < 80; y++){
			float s = 0;
			loop_for_fc_6:
			for (int k = 0; k < 16; k++){
				loop_for_fa_6:
				for (int j = 0; j < 5; j++){
					s=s+(kernel[16*5*n+5*k+j])*(Input_Conv[84*k+j+y*stride]);}
			}
			Output_Conv[80*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	write_to_file("output8.txt", Output_Conv, 1280);
	write_bias_to_file(bias, 16);
    write_weight_to_file(kernel, 1280);
	uint32_t pad_ctx = 2, n_ctx = 15, y_ctx = 3, k_ctx = 15, j_ctx = 4, alu_cfg_ctx = 1, stride_ctx = 0,s_ldm_ctx = 0, d_ldm_ctx = 2, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
void Add_1(float input_0[1280], float input_1[1280], float output[1280]) {
	for (int i = 0; i < 1280; i++){
		output[i] = input_0[i] + input_1[i];
	}
	uint32_t pad_ctx = 0, n_ctx = 15, y_ctx = 3, k_ctx = 0, j_ctx = 0, alu_cfg_ctx = 6, stride_ctx = 0,s_ldm_ctx = 2, d_ldm_ctx = 1, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
 void Activation3(float Input_Activation[1280], float Output_Activation[1280]){
	for (int i = 0; i < 1280; i++){
		if(Input_Activation[i] > 0){
			Output_Activation[i] = Input_Activation[i];
		}else
		{
			Output_Activation[i] = 0;
		}
	}
	write_to_file("output10.txt", Output_Activation, 1280);
}
void Padding_Conv1D_7(float input_Pad_Conv[1280], float output_Pad_Conv[1312]){
	loop_for_3_channel_pad_7:
	for (int c = 0; c < 16; c++){
		loop_for_channel_pad_7:
		for (int n = 0; n < 82; n++){
			if (n < 1 || n >= 81) output_Pad_Conv[82 * c + n]=0; else output_Pad_Conv[82 * c + n] = input_Pad_Conv[80 * c + n - 1];
		}
	}
}
void Conv1D_7(float Input_Conv[1312],float Output_Conv[1280], float bias[16], float kernel[768]){
	loop_for_channel_7:
	int stride = 1;
	for (int i = 0; i < 1312; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 16; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 768; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 16; n++){
		loop_for_ap_7:
		for (int y = 0; y < 80; y++){
			float s = 0;
			loop_for_fc_7:
			for (int k = 0; k < 16; k++){
				loop_for_fa_7:
				for (int j = 0; j < 3; j++){
					s=s+(kernel[16*3*n+3*k+j])*(Input_Conv[82*k+j+y*stride]);}
			}
			if ((s+bias[n])<0) Output_Conv[80*n+y]=0; else Output_Conv[80*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	write_to_file("output12.txt", Output_Conv, 1280);
	write_bias_to_file(bias, 16);
    write_weight_to_file(kernel, 768);
	uint32_t pad_ctx = 1, n_ctx = 15, y_ctx = 3, k_ctx = 15, j_ctx = 2, alu_cfg_ctx = 5, stride_ctx = 0,s_ldm_ctx = 2, d_ldm_ctx = 3, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
void Padding_Conv1D_8(float input_Pad_Conv[1280], float output_Pad_Conv[1344]){
	loop_for_3_channel_pad_8:
	for (int c = 0; c < 16; c++){
		loop_for_channel_pad_8:
		for (int n = 0; n < 84; n++){
			if (n < 2 || n >= 82) output_Pad_Conv[84 * c + n]=0; else output_Pad_Conv[84 * c + n] = input_Pad_Conv[80 * c + n - 2];
		}
	}
}
void Conv1D_8(float Input_Conv[1344],float Output_Conv[1280], float bias[16], float kernel[1280]){
	loop_for_channel_8:
	int stride = 1;
	for (int i = 0; i < 1344; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 16; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 1280; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 16; n++){
		loop_for_ap_8:
		for (int y = 0; y < 80; y++){
			float s = 0;
			loop_for_fc_8:
			for (int k = 0; k < 16; k++){
				loop_for_fa_8:
				for (int j = 0; j < 5; j++){
					s=s+(kernel[16*5*n+5*k+j])*(Input_Conv[84*k+j+y*stride]);}
			}
			Output_Conv[80*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	write_bias_to_file(bias, 16);
    write_weight_to_file(kernel, 1280);
	uint32_t pad_ctx = 2, n_ctx = 15, y_ctx = 3, k_ctx = 15, j_ctx = 4, alu_cfg_ctx = 5, stride_ctx = 0,s_ldm_ctx = 3, d_ldm_ctx = 0, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
 void Activation4(float Input_Activation[1280], float Output_Activation[1280]){
	for (int i = 0; i < 1280; i++){
		if(Input_Activation[i] > 0){
			Output_Activation[i] = Input_Activation[i];
		}else
		{
			Output_Activation[i] = 0;
		}
	}
}
void Padding_Conv1D_9(float input_Pad_Conv[1280], float output_Pad_Conv[1344]){
	loop_for_3_channel_pad_9:
	for (int c = 0; c < 16; c++){
		loop_for_channel_pad_9:
		for (int n = 0; n < 84; n++){
			if (n < 2 || n >= 82) output_Pad_Conv[84 * c + n]=0; else output_Pad_Conv[84 * c + n] = input_Pad_Conv[80 * c + n - 2];
		}
	}
}
void Conv1D_9(float Input_Conv[1344],float Output_Conv[1280], float bias[16], float kernel[1280]){
	loop_for_channel_9:
	int stride = 1;
	for (int i = 0; i < 1344; i++) {
		// printf("Input_Conv[%d] before: %f\n",i,Input_Conv[i]);
        Input_Conv[i] = fixedpoint_converter(Input_Conv[i]);
		// printf("Input_Conv[%d] after: %f\n",i,Input_Conv[i]);
    }
    for (int i = 0; i < 16; i++) {
        bias[i] = fixedpoint_converter(bias[i]);
    }
	for (int i = 0; i < 1280; i++) {
        kernel[i] = fixedpoint_converter(kernel[i]);
    }
	for (int n = 0; n < 16; n++){
		loop_for_ap_9:
		for (int y = 0; y < 80; y++){
			float s = 0;
			loop_for_fc_9:
			for (int k = 0; k < 16; k++){
				loop_for_fa_9:
				for (int j = 0; j < 5; j++){
					s=s+(kernel[16*5*n+5*k+j])*(Input_Conv[84*k+j+y*stride]);}
			}
			Output_Conv[80*n+y]=s+bias[n];
		}
	}
	for (int i = 0; i < 1280; i++) {
        Output_Conv[i] = fixedpoint_converter(Output_Conv[i]);
    }
	write_to_file("output14.txt", Output_Conv, 1280);
	write_bias_to_file(bias, 16);
    write_weight_to_file(kernel, 1280);
	uint32_t pad_ctx = 2, n_ctx = 15, y_ctx = 3, k_ctx = 15, j_ctx = 4, alu_cfg_ctx = 1, stride_ctx = 0,s_ldm_ctx = 0, d_ldm_ctx = 1, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
	fclose(weight_file);
	fclose(bias_file);
}
void Add_2(float input_0[1280], float input_1[1280], float output[1280]) {
	for (int i = 0; i < 1280; i++){
		output[i] = input_0[i] + input_1[i];
	}
	uint32_t pad_ctx = 0, n_ctx = 15, y_ctx = 3, k_ctx = 0, j_ctx = 0, alu_cfg_ctx = 6, stride_ctx = 0,s_ldm_ctx = 1, d_ldm_ctx = 0, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
 void Activation5(float Input_Activation[1280], float Output_Activation[1280]){
	for (int i = 0; i < 1280; i++){
		if(Input_Activation[i] > 0){
			Output_Activation[i] = Input_Activation[i];
		}else
		{
			Output_Activation[i] = 0;
		}
	}
	write_to_file("Output_Conv.txt", Output_Activation, 1280);
}
void Padding_Pool_0(float input_Pad_Pool[1280], float output_Pad_Pool[1296]){
	loop_for_3_channel_pad_0:
	for (int c = 0; c < 8; c++){
		loop_for_channel_pad_0:
		for (int n = 0; n < 162; n++){
			if (n < 1 || n >= 161) output_Pad_Pool[162 * c + n]=0; else output_Pad_Pool[162 * c + n] = input_Pad_Pool[160 * c + n - 1];
		}
	}
}
void Max_Pool1D_0(float input_MaxPooling[1296], float output_MaxPooling[1280]){
	int PoolSize = 3;
	int stride= 1;
	int index = 0;
	loop_for_channel_pool_0:
	for (int z = 0; z < 8; z++){
		index = 0;
		loop_for_weight_pool_0:
		for (int y = 0; y < 160; y++){
			float max = -10;
			for (int j = 0; j < PoolSize; j++)
			{
				int pool_index = 162 * z + j + y * stride;
				float pool_value = input_MaxPooling[pool_index];
				if (pool_value > max) max=pool_value;
			}
			int out_index = 160 * z + index;
			output_MaxPooling[out_index]=max;
			index++;
		}
	}
	write_to_file("output5.txt", output_MaxPooling, 1280);
	uint32_t pad_ctx = 1, n_ctx = 7, y_ctx = 7, k_ctx = 0, j_ctx = 0, alu_cfg_ctx = 3, stride_ctx = 0,s_ldm_ctx = 1, d_ldm_ctx = 2, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
void Padding_Pool_1(float input_Pad_Pool[1280], float output_Pad_Pool[1312]){
	loop_for_3_channel_pad_1:
	for (int c = 0; c < 16; c++){
		loop_for_channel_pad_1:
		for (int n = 0; n < 82; n++){
			if (n < 1 || n >= 81) output_Pad_Pool[82 * c + n]=0; else output_Pad_Pool[82 * c + n] = input_Pad_Pool[80 * c + n - 1];
		}
	}
}
void Max_Pool1D_1(float input_MaxPooling[1312], float output_MaxPooling[1280]){
	int PoolSize = 3;
	int stride= 1;
	int index = 0;
	loop_for_channel_pool_1:
	for (int z = 0; z < 16; z++){
		index = 0;
		loop_for_weight_pool_1:
		for (int y = 0; y < 80; y++){
			float max = -10;
			for (int j = 0; j < PoolSize; j++)
			{
				int pool_index = 82 * z + j + y * stride;
				float pool_value = input_MaxPooling[pool_index];
				if (pool_value > max) max=pool_value;
			}
			int out_index = 80 * z + index;
			output_MaxPooling[out_index]=max;
			index++;
		}
	}
	write_to_file("output11.txt", output_MaxPooling, 1280);
	uint32_t pad_ctx = 1, n_ctx = 15, y_ctx = 3, k_ctx = 0, j_ctx = 0, alu_cfg_ctx = 3, stride_ctx = 0,s_ldm_ctx = 1, d_ldm_ctx = 2, sa_ldm_ctx = 0;
    uint32_t result = concatenate_to_32bit(pad_ctx, n_ctx, y_ctx, k_ctx, j_ctx, alu_cfg_ctx, stride_ctx, s_ldm_ctx, d_ldm_ctx, sa_ldm_ctx);
	write_context_to_file( &result, 1);
}
void GlobalAveragePool1D_0(float input_GlobalAveragePool1D[1280],float output_GlobalAveragePool1D[16]){
	int hs = 0;
	for (int i = 0; i < 16; i++){
		float avg = 0;
		for (int j = 0; j < 80; j++){
			avg += input_GlobalAveragePool1D[80 * i + j] / 80;
		}
		output_GlobalAveragePool1D[hs] = avg;
		hs++;
	}
}
