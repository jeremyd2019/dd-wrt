/*
 * misagent.c
 * com.apple.misagent service implementation.
 *
 * Copyright (c) 2012 Nikias Bassen, All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <plist/plist.h>
#include <stdio.h>

#include "misagent.h"
#include "property_list_service.h"
#include "debug.h"

/**
 * Convert a property_list_service_error_t value to a misagent_error_t
 * value. Used internally to get correct error codes.
 *
 * @param err A property_list_service_error_t error code
 *
 * @return A matching misagent_error_t error code,
 *     MISAGENT_E_UNKNOWN_ERROR otherwise.
 */
static misagent_error_t misagent_error(property_list_service_error_t err)
{
       switch (err) {
                case PROPERTY_LIST_SERVICE_E_SUCCESS:
                        return MISAGENT_E_SUCCESS;
                case PROPERTY_LIST_SERVICE_E_INVALID_ARG:
                        return MISAGENT_E_INVALID_ARG;
                case PROPERTY_LIST_SERVICE_E_PLIST_ERROR:
                        return MISAGENT_E_PLIST_ERROR;
                case PROPERTY_LIST_SERVICE_E_MUX_ERROR:
                        return MISAGENT_E_CONN_FAILED;
                default:
                        break;
        }
        return MISAGENT_E_UNKNOWN_ERROR;
}

/**
 * Checks the response from misagent to determine if the operation
 * was successful or an error occured. Internally used only.
 *
 * @param response a PLIST_DICT received from device's misagent
 * @param status_code pointer to an int that will be set to the status code
 *   contained in the response
 */
static misagent_error_t misagent_check_result(plist_t response, int* status_code)
{
	if (plist_get_node_type(response) != PLIST_DICT) {
		return MISAGENT_E_PLIST_ERROR;
	}

	plist_t node = plist_dict_get_item(response, "Status");
	if (!node || (plist_get_node_type(node) != PLIST_UINT)) {
		return MISAGENT_E_PLIST_ERROR;
	}

	uint64_t val = -1LL;
	plist_get_uint_val(node, &val);
	if ((int64_t)val == -1LL) {
		return MISAGENT_E_PLIST_ERROR;
	}
	*status_code = (int)(val & 0xFFFFFFFF);
	if (*status_code == 0) {
		return MISAGENT_E_SUCCESS;
	} else {
		return MISAGENT_E_REQUEST_FAILED;
	}
}

/**
 * Connects to the misagent service on the specified device.
 *
 * @param device The device to connect to.
 * @param service The service descriptor returned by lockdownd_start_service.
 * @param client Pointer that will point to a newly allocated
 *     misagent_client_t upon successful return.
 *
 * @return MISAGENT_E_SUCCESS on success, MISAGENT_E_INVALID_ARG when
 *     client is NULL, or an MISAGENT_E_* error code otherwise.
 */
misagent_error_t misagent_client_new(idevice_t device, lockdownd_service_descriptor_t service, misagent_client_t *client)
{
	property_list_service_client_t plistclient = NULL;
	misagent_error_t err = misagent_error(property_list_service_client_new(device, service, &plistclient));
	if (err != MISAGENT_E_SUCCESS) {
		return err;
	}

	misagent_client_t client_loc = (misagent_client_t) malloc(sizeof(struct misagent_client_private));
	client_loc->parent = plistclient;
	client_loc->last_error = 0;

	*client = client_loc;
	return MISAGENT_E_SUCCESS;
}

/**
 * Disconnects an misagent client from the device and frees up the
 * misagent client data.
 *
 * @param client The misagent client to disconnect and free.
 *
 * @return MISAGENT_E_SUCCESS on success, MISAGENT_E_INVALID_ARG when
 *     client is NULL, or an MISAGENT_E_* error code otherwise.
 */
misagent_error_t misagent_client_free(misagent_client_t client)
{
	if (!client)
		return MISAGENT_E_INVALID_ARG;

	misagent_error_t err = MISAGENT_E_SUCCESS;
	if (client->parent && client->parent->parent) {
		misagent_error(property_list_service_client_free(client->parent));
	}
	client->parent = NULL;
	free(client);

	return err;
}

/**
 * Installs the given provisioning profile. Only works with valid profiles.
 *
 * @param client The connected misagent to use for installation
 * @param profile The valid provisioning profile to install. This has to be
 *    passed as a PLIST_DATA, otherwise the function will fail.
 *
 * @return MISAGENT_E_SUCCESS on success, MISAGENT_E_INVALID_ARG when
 *     client is invalid, or an MISAGENT_E_* error code otherwise.
 */
misagent_error_t misagent_install(misagent_client_t client, plist_t profile)
{
	if (!client || !client->parent || !profile || (plist_get_node_type(profile) != PLIST_DATA))
                return MISAGENT_E_INVALID_ARG;

	client->last_error = MISAGENT_E_UNKNOWN_ERROR;

	plist_t dict = plist_new_dict();
	plist_dict_insert_item(dict, "MessageType", plist_new_string("Install"));
	plist_dict_insert_item(dict, "Profile", plist_copy(profile));
	plist_dict_insert_item(dict, "ProfileType", plist_new_string("Provisioning"));

	misagent_error_t res = misagent_error(property_list_service_send_xml_plist(client->parent, dict));
	plist_free(dict);
	dict = NULL;

        if (res != MISAGENT_E_SUCCESS) {
                debug_info("could not send plist, error %d", res);
		return res;
        }

	res = misagent_error(property_list_service_receive_plist(client->parent, &dict));
	if (res != MISAGENT_E_SUCCESS) {
		debug_info("could not receive response, error %d", res);
		return res;
	}
	if (!dict) {
		debug_info("could not get response plist");
		return MISAGENT_E_UNKNOWN_ERROR;
	}

	res = misagent_check_result(dict, &client->last_error);
	plist_free(dict);

	return res;
}

/**
 * Retrieves an array of all installed provisioning profiles.
 *
 * @param client The connected misagent to use.
 * @param profiles Pointer to a plist_t that will be set to a PLIST_ARRAY
 *    if the function is successful.
 *
 * @return MISAGENT_E_SUCCESS on success, MISAGENT_E_INVALID_ARG when
 *     client is invalid, or an MISAGENT_E_* error code otherwise.
 *
 * @note If no provisioning profiles are installed on the device, this function
 *     still returns MISAGENT_E_SUCCESS and profiles will just point to an
 *     empty array.
 */
misagent_error_t misagent_copy(misagent_client_t client, plist_t* profiles)
{
	if (!client || !client->parent || !profiles)
                return MISAGENT_E_INVALID_ARG;

	client->last_error = MISAGENT_E_UNKNOWN_ERROR;

	plist_t dict = plist_new_dict();
	plist_dict_insert_item(dict, "MessageType", plist_new_string("Copy"));
	plist_dict_insert_item(dict, "ProfileType", plist_new_string("Provisioning"));

	misagent_error_t res = misagent_error(property_list_service_send_xml_plist(client->parent, dict));
	plist_free(dict);
	dict = NULL;

        if (res != MISAGENT_E_SUCCESS) {
                debug_info("could not send plist, error %d", res);
		return res;
        }

	res = misagent_error(property_list_service_receive_plist(client->parent, &dict));
	if (res != MISAGENT_E_SUCCESS) {
		debug_info("could not receive response, error %d", res);
		return res;
	}
	if (!dict) {
		debug_info("could not get response plist");
		return MISAGENT_E_UNKNOWN_ERROR;
	}

	res = misagent_check_result(dict, &client->last_error);
	if (res == MISAGENT_E_SUCCESS) {
		*profiles = plist_copy(plist_dict_get_item(dict, "Payload"));
	}
	plist_free(dict);

	return res;

}

/**
 * Removes a given provisioning profile.
 *
 * @param client The connected misagent to use.
 * @param profileID Identifier of the provisioning profile to remove.
 *    This is a UUID that can be obtained from the provisioning profile data.
 * @see misagent_copy
 *
 * @return MISAGENT_E_SUCCESS on success, MISAGENT_E_INVALID_ARG when
 *     client is invalid, or an MISAGENT_E_* error code otherwise.
 */
misagent_error_t misagent_remove(misagent_client_t client, const char* profileID)
{
	if (!client || !client->parent || !profileID)
                return MISAGENT_E_INVALID_ARG;

	client->last_error = MISAGENT_E_UNKNOWN_ERROR;

	plist_t dict = plist_new_dict();
	plist_dict_insert_item(dict, "MessageType", plist_new_string("Remove"));
	plist_dict_insert_item(dict, "ProfileID", plist_new_string(profileID));
	plist_dict_insert_item(dict, "ProfileType", plist_new_string("Provisioning"));

	misagent_error_t res = misagent_error(property_list_service_send_xml_plist(client->parent, dict));
	plist_free(dict);
	dict = NULL;

        if (res != MISAGENT_E_SUCCESS) {
                debug_info("could not send plist, error %d", res);
		return res;
        }

	res = misagent_error(property_list_service_receive_plist(client->parent, &dict));
	if (res != MISAGENT_E_SUCCESS) {
		debug_info("could not receive response, error %d", res);
		return res;
	}
	if (!dict) {
		debug_info("could not get response plist");
		return MISAGENT_E_UNKNOWN_ERROR;
	}

	res = misagent_check_result(dict, &client->last_error);
	plist_free(dict);

	return res;
}

/**
 * Retrieves the status code from the last operation.
 *
 * @param client The misagent to use.
 *
 * @return -1 if client is invalid, or the status code from the last operation
 */
int misagent_get_status_code(misagent_client_t client)
{
	if (!client) {
		return -1;
	}
	return client->last_error;
}
