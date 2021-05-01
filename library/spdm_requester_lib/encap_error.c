/**
    Copyright Notice:
    Copyright 2021 DMTF. All rights reserved.
    License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libspdm/blob/main/LICENSE.md
**/

#include "spdm_requester_lib_internal.h"

/**
  Generate encapsulated ERROR message.

  This function can be called in spdm_get_encap_response_func.

  @param  spdm_context                  A pointer to the SPDM context.
  @param  error_code                    The error code of the message.
  @param  error_data                    The error data of the message.
  @param  spdm_response_size             size in bytes of the response data.
                                       On input, it means the size in bytes of data buffer.
                                       On output, it means the size in bytes of copied data buffer if RETURN_SUCCESS is returned,
                                       and means the size in bytes of desired data buffer if RETURN_BUFFER_TOO_SMALL is returned.
  @param  spdm_response                 A pointer to the response data.

  @retval RETURN_SUCCESS               The error message is generated.
  @retval RETURN_BUFFER_TOO_SMALL      The buffer is too small to hold the data.
**/
return_status spdm_generate_encap_error_response(IN void *context,
						 IN uint8 error_code,
						 IN uint8 error_data,
						 IN OUT uintn *response_size,
						 OUT void *response)
{
	spdm_error_response_t *spdm_response;

	ASSERT(*response_size >= sizeof(spdm_error_response_t));
	*response_size = sizeof(spdm_error_response_t);
	spdm_response = response;

	spdm_response->header.spdm_version = SPDM_MESSAGE_VERSION_11;
	spdm_response->header.request_response_code = SPDM_ERROR;
	spdm_response->header.param1 = error_code;
	spdm_response->header.param2 = error_data;

	return RETURN_SUCCESS;
}

/**
  Generate encapsulated ERROR message with extended error data.

  This function can be called in spdm_get_encap_response_func.

  @param  spdm_context                  A pointer to the SPDM context.
  @param  error_code                    The error code of the message.
  @param  error_data                    The error data of the message.
  @param  extended_error_data_size        The size in bytes of the extended error data.
  @param  extended_error_data            A pointer to the extended error data.
  @param  spdm_response_size             size in bytes of the response data.
                                       On input, it means the size in bytes of response data buffer.
                                       On output, it means the size in bytes of copied response data buffer if RETURN_SUCCESS is returned,
                                       and means the size in bytes of desired response data buffer if RETURN_BUFFER_TOO_SMALL is returned.
  @param  spdm_response                 A pointer to the response data.

  @retval RETURN_SUCCESS               The error message is generated.
  @retval RETURN_BUFFER_TOO_SMALL      The buffer is too small to hold the data.
**/
return_status spdm_generate_encap_extended_error_response(
	IN void *context, IN uint8 error_code, IN uint8 error_data,
	IN uintn extended_error_data_size, IN uint8 *extended_error_data,
	IN OUT uintn *response_size, OUT void *response)
{
	spdm_error_response_t *spdm_response;

	ASSERT(*response_size >=
	       sizeof(spdm_error_response_t) + extended_error_data_size);
	*response_size =
		sizeof(spdm_error_response_t) + extended_error_data_size;
	spdm_response = response;

	spdm_response->header.spdm_version = SPDM_MESSAGE_VERSION_11;
	spdm_response->header.request_response_code = SPDM_ERROR;
	spdm_response->header.param1 = error_code;
	spdm_response->header.param2 = error_data;
	copy_mem(spdm_response + 1, extended_error_data,
		 extended_error_data_size);

	return RETURN_SUCCESS;
}
