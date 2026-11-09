#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
#include "time_link.h"
#include "pin_function_def.h"
#include "fr_698_esam.h"
#include "io_type.h"              
#include "sm3.h"
#include "sm4.h"


#ifdef __DEBUG_ESAM__

uint8_t get_mac(uint8_t*input, uint16_t len, uint8_t* mac)
{
	int  i = 0;
	uint8_t output[32] = {0};

	SM3Calc(input, len, output);

#if 0
	printf("output Hash:\n   ");
	for (i = 0; i < 32; i++)
	{
		printf("%02x", output[i]);
		if (((i + 1) % 4) == 0) printf(" ");
	}
	printf("\n");
#endif
	for (i = 0; i < 8; i++)
	{
		mac[0] ^= output[i * 4];
		mac[1] ^= output[i * 4+1];
		mac[2] ^= output[i * 4+2];
		mac[3] ^= output[i * 4+3];
	}

	//printf("mac: %02x-%02x-%02x-%02x\n", mac[0], mac[1], mac[2], mac[3]);
	return 4;

}

int encrypt_or_decrypt(uint8_t *input, uint8_t *output, uint8_t type)
{
	unsigned char key[16] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 };

	sm4_context ctx;
	unsigned long i;

	if (1 == type) {
		//encrypt standard testing vector
		sm4_setkey_enc(&ctx, key);
		sm4_crypt_ecb(&ctx, 1, 16, input, output);
		//for (i = 0; i < 16; i++)
		//	printf("%02x ", output[i]);
		//printf("\n");
	}
	if (0 == type) {
		//decrypt testing
		sm4_setkey_dec(&ctx, key);
		sm4_crypt_ecb(&ctx, 0, 16, output, output);
		//for (i = 0; i < 16; i++)
		//	printf("%02x ", output[i]);
		//printf("\n");
	}
    return 0;
}	

int m_test()
{
	uint8_t input[16] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 };
	int ilen = 16;
	int  i = 0;
    uint8_t output[32] = {0};
	uint8_t data[32] = { 0 };
	uint8_t mac[4] = { 0 };
	uint8_t mac_2[4] = { 0 };
    
	uint8_t mac_len = get_mac(input, ilen, mac);
#if 0
	printf("input:\n   ");
	for (i = 0; i < 16; i++)
	{
		printf("%02x ", input[i]);
	}
	printf("\n");
	printf("mac1:\n   ");
	for (i = 0; i < 4; i++)
	{
		printf("%02x ", mac[i]);
	}
	printf("\n");
#endif
	encrypt_or_decrypt(input, output, 1);
	memcpy(data, output, 16);
	memcpy(data + 16, mac, 4);

	encrypt_or_decrypt(data, output, 0);
	uint8_t mac_len_2 = get_mac(output, 16, mac_2);
#if 0
	printf("output:\n   ");
	for (i = 0; i < 16; i++)
	{
		printf("%02x ", output[i]);
	}
	printf("\n");
	printf("mac2:\n   ");
	for (i = 0; i < 4; i++)
	{
		printf("%02x ", mac_2[i]);
	}
	printf("\n");
#endif
	if (0 == memcmp(mac, mac_2, 4)) {
		if (0 == memcmp(input, output, 16)) {
			//printf("test sm ok!!!", output[i]);
          output[0] = output[1];
		}
	}
    return 0;
}

int openM_ioctl(int fd, int request, void *param)
{
    struct esam_data_t *esam_data = (struct esam_data_t *)param;   
    uint16_t len = 0;
    int ret = 0;
    
    if(esam_poweron_cmd == request){
        return 1;
    }
    
    if(esam_poweroff_cmd == request){
        return 1;
    }
    
    if(esam_comm_handle_cmd == request){
        //tx_esam(esam_data->head , esam_data->input->data_buf , esam_data->len , NULL);
        //ret = rx_esam(esam_data->input->data_buf , &len , NULL , &esam_data->sw);    
        if(esam_data->sw == 0x9000){
            esam_data->len = len;
            return 1;
        }
    }
    return 0;
}

#endif
