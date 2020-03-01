#include "hk_pairing_ble.h"

#include "../../utils/hk_tlv.h"
#include "../../include/hk_mem.h"
#include "../../common/hk_pair_setup.h"

const uint8_t hk_paring_ble_features = 0; //zero because non mfi certified
hk_mem *hk_pairing_ble_device_id = NULL;

#include "../../utils/hk_logging.h"

void hk_pairing_ble_read_pair_setup(hk_mem* response)
{
    HK_LOGE("hk_pairing_ble_read_pair_setup");
}

void hk_pairing_ble_write_pair_setup(hk_mem* request, hk_mem* response)
{    
    hk_pairing_ble_device_id = hk_mem_create();
    
    int res = hk_pair_setup(request, response, hk_pairing_ble_device_id);
    if(res != 0){
        HK_LOGE("Error in pair setup: %d", res);
    }
}

void hk_pairing_ble_read_pair_verify(hk_mem* response)
{
    HK_LOGE("hk_pairing_ble_read_pair_verify");
}

void hk_pairing_ble_write_pair_verify(hk_mem* request, hk_mem* response)
{
    HK_LOGE("hk_pairing_ble_write_pair_verify");
}

void hk_pairing_ble_read_pairing_features(hk_mem* response)
{
    hk_mem_append_buffer(response, (void*)&hk_paring_ble_features, sizeof(uint8_t));
    return ;
}

void hk_pairing_ble_read_pairing_pairings(hk_mem* response)
{
    HK_LOGE("hk_pairing_ble_read_pairing_pairings");
}

void hk_pairing_ble_write_pairing_pairings(hk_mem* request, hk_mem* response)
{
    HK_LOGE("hk_pairing_ble_write_pairing_pairings");
}