#include <cmath>  // Include cmath for the round function
#include <cstdio>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
FILE *file1 = fopen("WRAM_2_File.txt", "w");	
FILE *file2 = fopen("BRAM_2_File.txt", "w");
void Dense_0(float input_Dense[16],float &output_Dense0,float bias[3],float weight[48]){
	float out_Dense[3];
	for (int i = 0; i < 48; i++) {
        fprintf(file1, "%f\n", weight[i]);
    }

    fclose(file1);

    for (int i = 0; i < 3; i++) {
        fprintf(file2, "%f\n", bias[i]);
    }

    fclose(file2);
	loop_for_a_Dense_0:
	for (int i = 0; i < 3; i++){
		float s=0;
		loop_for_b_Dense_0:
		for (int j = 0; j < 16; j++){
			s+=input_Dense[j]*weight[j*3+i];
		}
		out_Dense[i]=s+bias[i];
	}
	int maxindex = 0;
	float max=out_Dense[0];
	loop_detect:
	for (int i=0; i<3; i++){
		if (out_Dense[i]> max) {
			max=out_Dense[i];
			maxindex=i;
		}
	}
	float sum_exp_x = 0.0;
	for(int i = 0; i <3;i++){
		sum_exp_x += exp(out_Dense[i]- out_Dense[maxindex]);
	}
	float max_value = out_Dense[maxindex];
	for(int i = 0; i <3;i++){
		out_Dense[i] = exp(out_Dense[i] - max_value) / sum_exp_x;
	}
	float maxindex_2 = 0;
	float max_2 = out_Dense[0];
	for(int i = 0; i <3;i++){
		if (out_Dense[i] > max_2) {
			max_2 = out_Dense[i];
			maxindex_2 = i;
		}
	}
	output_Dense0 = maxindex_2;
}
