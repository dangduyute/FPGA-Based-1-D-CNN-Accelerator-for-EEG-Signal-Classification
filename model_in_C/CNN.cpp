#include "Conv.h"
#include "Pool.h"
#include "Dense.h"
#include <algorithm>
#include <string.h>
void CNN(float InModel[320],float &OutModel0,float Weights[7243]){
	float OutPadConv0[325];
	float conv1d_54[640];
	float OutPadConv1[656];
	float conv1d_55[1280];
	float re_lu_30[1280];
	float OutPadConv2[1312];
	float conv1d_56[1280];
	float OutPadConv3[640];
	float conv1d_57[1280];
	float add_15[1280];
	float re_lu_31[1280];
	float OutPadPool0[1296];
	float max_pooling1d_10[1280];
	float OutPadConv4[1304];
	float conv1d_58[1280];
	float OutPadConv5[1344];
	float conv1d_59[1280];
	float re_lu_32[1280];
	float OutPadConv6[1344];
	float conv1d_60[1280];
	float add_16[1280];
	float re_lu_33[1280];
	float OutPadPool1[1312];
	float max_pooling1d_11[1280];
	float OutPadConv7[1312];
	float conv1d_61[1280];
	float OutPadConv8[1344];
	float conv1d_62[1280];
	float re_lu_34[1280];
	float OutPadConv9[1344];
	float conv1d_63[1280];
	float add_17[1280];
	float re_lu_35[1280];
	float global_average_pooling1d_5[16];
	Padding_Conv1D_0(&InModel[0],OutPadConv0);
	Conv1D_0(OutPadConv0,conv1d_54,&Weights[28],&Weights[0]);
	Padding_Conv1D_1(conv1d_54,OutPadConv1);
	Conv1D_1(OutPadConv1,conv1d_55,&Weights[192],&Weights[32]);
	Activation0(conv1d_55,re_lu_30);
	Padding_Conv1D_2(re_lu_30,OutPadConv2);
	Conv1D_2(OutPadConv2,conv1d_56,&Weights[520],&Weights[200]);
	Padding_Conv1D_3(conv1d_54,OutPadConv3);
	Conv1D_3(OutPadConv3,conv1d_57,&Weights[560],&Weights[528]);
	Add_0(conv1d_56, conv1d_57, add_15);
	Activation1(add_15,re_lu_31);
	Padding_Pool_0(re_lu_31,OutPadPool0);
	Max_Pool1D_0(OutPadPool0,max_pooling1d_10);
	Padding_Conv1D_4(max_pooling1d_10,OutPadConv4);
	Conv1D_4(OutPadConv4,conv1d_58,&Weights[1208],&Weights[568]);
	Padding_Conv1D_5(conv1d_58,OutPadConv5);
	Conv1D_5(OutPadConv5,conv1d_59,&Weights[2504],&Weights[1224]);
	Activation2(conv1d_59,re_lu_32);
	Padding_Conv1D_6(re_lu_32,OutPadConv6);
	Conv1D_6(OutPadConv6,conv1d_60,&Weights[3800],&Weights[2520]);
	Add_1(conv1d_60, conv1d_58, add_16);
	Activation3(add_16,re_lu_33);
	Padding_Pool_1(re_lu_33,OutPadPool1);
	Max_Pool1D_1(OutPadPool1,max_pooling1d_11);
	Padding_Conv1D_7(max_pooling1d_11,OutPadConv7);
	Conv1D_7(OutPadConv7,conv1d_61,&Weights[4584],&Weights[3816]);
	Padding_Conv1D_8(conv1d_61,OutPadConv8);
	Conv1D_8(OutPadConv8,conv1d_62,&Weights[5880],&Weights[4600]);
	Activation4(conv1d_62,re_lu_34);
	Padding_Conv1D_9(re_lu_34,OutPadConv9);
	Conv1D_9(OutPadConv9,conv1d_63,&Weights[7176],&Weights[5896]);
	Add_2(conv1d_63, conv1d_61, add_17);
	Activation5(add_17,re_lu_35);
	GlobalAveragePool1D_0(re_lu_35,global_average_pooling1d_5);
	Dense_0(global_average_pooling1d_5,OutModel0,&Weights[7240],&Weights[7192]);
}
