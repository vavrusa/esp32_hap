#include "hk_gap.h"

#include <services/gap/ble_svc_gap.h>

#include "../../utils/hk_logging.h"
#include "../../utils/hk_store.h"
#include "../../common/hk_accessory_id.h"
#include "../../common/hk_pairings_store.h"
#include "../../common/hk_global_state.h"

#include "hk_connection_security.h"
#include "hk_connection.h"

static uint8_t hk_gap_own_addr_type;
const char *hk_gap_name; // todo: move to config
size_t hk_gap_category;  // todo: move to config, type to char

static void hk_gap_connect(uint16_t connection_handle)
{
    hk_connection_init(connection_handle);
}

static void hk_gap_disconnect(uint16_t connection_handle)
{
    hk_connection_free(connection_handle);
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int hk_gap_gap_event(struct ble_gap_event *event, void *arg)
{
    int rc = 0;

    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        HK_LOGD("Connect event; status=%d ", event->connect.status);
        if (event->connect.status == 0)
        {
            hk_gap_connect(event->connect.conn_handle);
        }
        else
        {
            //Connection failed; resume advertising.
            hk_gap_start_advertising(hk_gap_own_addr_type);
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        HK_LOGD("Disconnect event; reason=%d ", event->disconnect.reason);
        hk_gap_disconnect(event->disconnect.conn.conn_handle);
        hk_gap_start_advertising(hk_gap_own_addr_type);
        break;
    case BLE_GAP_EVENT_CONN_UPDATE:
        HK_LOGV("connection updated; status=%d ", event->conn_update.status);
        rc = 0;
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        HK_LOGD("advertise complete; reason=%d", event->adv_complete.reason);
        hk_gap_start_advertising(hk_gap_own_addr_type);
        rc = 0;
        break;
    case BLE_GAP_EVENT_ENC_CHANGE:
        HK_LOGD("encryption change event; status=%d ", event->enc_change.status);
        rc = 0;
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        HK_LOGV("subscribe event; conn_handle=%d attr_handle=%d "
                "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                event->subscribe.conn_handle,
                event->subscribe.attr_handle,
                event->subscribe.reason,
                event->subscribe.prev_notify,
                event->subscribe.cur_notify,
                event->subscribe.prev_indicate,
                event->subscribe.cur_indicate);
        rc = 0;
        break;
    case BLE_GAP_EVENT_MTU:
        HK_LOGV("mtu update event; conn_handle=%d cid=%d mtu=%d",
                event->mtu.conn_handle,
                event->mtu.channel_id,
                event->mtu.value);
        hk_connection_mtu_set(event->mtu.conn_handle, event->mtu.value);
        rc = 0;
        break;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        HK_LOGD("Repeat pairing");
        rc = 0;
        break;
    case BLE_GAP_EVENT_PASSKEY_ACTION:
        HK_LOGD("PASSKEY_ACTION_EVENT started");
        rc = 0;
        break;
    }

    return rc;
}

void hk_gap_set_address(uint8_t own_addr_type)
{
    hk_gap_own_addr_type = own_addr_type;
}

void hk_gap_start_advertising()
{
    HK_LOGV("Starting advertising.");
    int res;

    hk_mem *data = hk_mem_init();
    hk_accessory_id_get(data);

    uint16_t global_state = hk_global_state_get();
    bool has_pairing = false;
    hk_pairings_store_has_pairing(&has_pairing);

    uint8_t manufacturer_data[17];
    manufacturer_data[0] = 0x4c;                           // company id
    manufacturer_data[1] = 0x00;                           // company id
    manufacturer_data[2] = 0x06;                           // type
    manufacturer_data[3] = 0xcd;                           // subtype and length
    manufacturer_data[4] = has_pairing ? 0x00 : 0x01;      // pairing status flat
    manufacturer_data[5] = data->ptr[0];                   // device id
    manufacturer_data[6] = data->ptr[1];                   // device id
    manufacturer_data[7] = data->ptr[2];                   // device id
    manufacturer_data[8] = data->ptr[3];                   // device id
    manufacturer_data[9] = data->ptr[4];                   // device id
    manufacturer_data[10] = data->ptr[5];                  // device id
    manufacturer_data[11] = (char)hk_gap_category; // accessory category identifier
    manufacturer_data[12] = 0x00;                          // accessory category identifier // todo: make custom
    manufacturer_data[13] = global_state % 256;            // global state number
    manufacturer_data[14] = global_state / 256;            // global state number
    manufacturer_data[15] = hk_store_configuration_get();  // configuration number
    manufacturer_data[16] = 0x02;                          // HAP BLE version

    HK_LOGV("With status flag sf: %d, global state: %d, configuration: %d", manufacturer_data[4], global_state, hk_store_configuration_get());
    hk_mem_free(data);
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof fields);
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP; // Discoverability in forthcoming advertisement (general) // BLE-only (BR/EDR unsupported)
    fields.name = (uint8_t *)hk_gap_name;
    fields.name_len = strlen(hk_gap_name);
    fields.name_is_complete = 1;
    fields.mfg_data = manufacturer_data;
    fields.mfg_data_len = sizeof(manufacturer_data);

    res = ble_gap_adv_set_fields(&fields);
    if (res != 0)
    {
        HK_LOGE("Could not start advertising, because fields could not be set. Errorcode: %d", res);
        return;
    }

    /* Begin advertising. */
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    //adv_params.itvl_min = 20;
    res = ble_gap_adv_start(hk_gap_own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, hk_gap_gap_event, NULL);
    if (res != 0)
    {
        HK_LOGE("Could not start advertising. Errorcode: %d", res);
        return;
    }
}

void hk_gap_stop_advertising()
{
    int res = ble_gap_adv_stop();
    if (res != 0)
    {
        HK_LOGE("Could not stop advertising. Errorcode: %d", res);
    }
}

void hk_gap_init(const char *name, size_t category, size_t config_version)
{
    HK_LOGD("Initializing GAP.");
    ble_svc_gap_init();
    hk_gap_name = name;
    hk_gap_category = category;
    int res = ble_svc_gap_device_name_set(name);
    if (res != ESP_OK)
    {
        HK_LOGE("Error setting name for advertising.");
        return;
    }
}

void hk_gap_terminate_connection(uint16_t connection_handle)
{
    ble_gap_terminate(connection_handle, BLE_ERR_REM_USER_CONN_TERM);
}