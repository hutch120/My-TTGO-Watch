/****************************************************************************
 *   Tu May 22 21:23:51 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <Arduino.h>
#include <esp_now.h>

#include "../hardware/wifictl.h"
#include "hardware/powermgm.h"

bool espnow_init = false;

bool espnow_wifictl_event_cb(EventBits_t event, void *arg);
void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
void broadcast(const String &message);

void espnow_start()
{
    if (espnow_init == true)
        return;
    espnow_init = true;

    if (esp_now_init() == ESP_OK)
    {
        log_i("******************* ESPNow Init Success ... register send/recv callbacks...");
        esp_now_register_recv_cb(receiveCallback);
        esp_now_register_send_cb(sentCallback);
        wifictl_register_cb(WIFICTL_ON, espnow_wifictl_event_cb, "handle espnow");
    }
    else
    {
        log_i("******************* ESPNow Init Failed");
    }
}

bool espnow_wifictl_event_cb(EventBits_t event, void *arg)
{
    switch (event)
    {
    case WIFICTL_ON:
        log_i("******************* ESPNow broadcast message");
        broadcast("hi");
        break;
    }
    return (true);
}

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
{
    snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
    // only allow a maximum of 250 characters in the message + a null terminating byte
    char buffer[ESP_NOW_MAX_DATA_LEN + 1];
    int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
    strncpy(buffer, (const char *)data, msgLen);
    // make sure we are null terminated
    buffer[msgLen] = 0;
    // format the mac address
    char macStr[18];
    formatMacAddress(macAddr, macStr, 18);
    // debug log the message to the serial port
    log_i("******************* Received message from: %s - %s", macStr, buffer);
    // what are our instructions
    if (strcmp("hi", buffer) == 0)
    {
        log_i("******************* ESPNOW test received");
    }
    else
    {
        log_i("******************* ESPNOW some message received");
    }
}

// callback when data is sent
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
{
    char macStr[18];
    formatMacAddress(macAddr, macStr, 18);
    log_i("******************* Last Packet Sent to: %s", macStr);
    log_i("******************* Last Packet Send Status: %s", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void broadcast(const String &message)
{
    // this will broadcast a message to everyone in range
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
    if (!esp_now_is_peer_exist(broadcastAddress))
    {
        esp_now_add_peer(&peerInfo);
    }
    esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
    // and this will send a message to a specific device
    /*uint8_t peerAddress[] = {0x3C, 0x71, 0xBF, 0x47, 0xA5, 0xC0};
        esp_now_peer_info_t peerInfo = {};
        memcpy(&peerInfo.peer_addr, peerAddress, 6);
        if (!esp_now_is_peer_exist(peerAddress))
        {
            esp_now_add_peer(&peerInfo);
        }
        esp_err_t result = esp_now_send(peerAddress, (const uint8_t *)message.c_str(), message.length());
    */
    if (result == ESP_OK)
    {
        log_i("******************* Broadcast message success");
    }
    else if (result == ESP_ERR_ESPNOW_NOT_INIT)
    {
        log_i("******************* ESPNOW not Init.");
    }
    else if (result == ESP_ERR_ESPNOW_ARG)
    {
        log_i("******************* Invalid Argument");
    }
    else if (result == ESP_ERR_ESPNOW_INTERNAL)
    {
        log_i("******************* Internal Error");
    }
    else if (result == ESP_ERR_ESPNOW_NO_MEM)
    {
        log_i("******************* ESP_ERR_ESPNOW_NO_MEM");
    }
    else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
    {
        log_i("******************* Peer not found.");
    }
    else
    {
        log_i("******************* Unknown error");
    }
}