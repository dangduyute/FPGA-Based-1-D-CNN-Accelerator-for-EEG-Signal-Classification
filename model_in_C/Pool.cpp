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
