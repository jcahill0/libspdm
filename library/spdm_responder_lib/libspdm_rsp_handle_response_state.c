/**
    Copyright Notice:
    Copyright 2021 DMTF. All rights reserved.
    License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libspdm/blob/main/LICENSE.md
**/

#include "internal/libspdm_responder_lib.h"

/**
  Build the response when the response state is incorrect.

  @param  spdm_context                  A pointer to the SPDM context.
  @param  request_code                  The SPDM request code.
  @param  response_size                 size in bytes of the response data.
                                       On input, it means the size in bytes of response data buffer.
                                       On output, it means the size in bytes of copied response data buffer if RETURN_SUCCESS is returned,
                                       and means the size in bytes of desired response data buffer if RETURN_BUFFER_TOO_SMALL is returned.
  @param  response                     A pointer to the response data.

  @retval RETURN_SUCCESS               The response is returned.
  @retval RETURN_BUFFER_TOO_SMALL      The buffer is too small to hold the data.
  @retval RETURN_DEVICE_ERROR          A device error occurs when communicates with the device.
  @retval RETURN_SECURITY_VIOLATION    Any verification fails.
**/
return_status spdm_responder_handle_response_state(IN void *context,
						   IN uint8 request_code,
						   IN OUT uintn *response_size,
						   OUT void *response)
{
	spdm_context_t *spdm_context;

	spdm_context = context;
	switch (spdm_context->response_state) {
	case SPDM_RESPONSE_STATE_BUSY:
		libspdm_generate_error_response(spdm_context, SPDM_ERROR_CODE_BUSY,
					     0, response_size, response);
		// NOTE: Need to reset status to Normal in up level
		return RETURN_SUCCESS;
	case SPDM_RESPONSE_STATE_NEED_RESYNC:
		libspdm_generate_error_response(spdm_context,
					     SPDM_ERROR_CODE_REQUEST_RESYNCH, 0,
					     response_size, response);
		// NOTE: Need to let SPDM_VERSION reset the State
		spdm_set_connection_state(spdm_context,
					  SPDM_CONNECTION_STATE_NOT_STARTED);
		return RETURN_SUCCESS;
	case SPDM_RESPONSE_STATE_NOT_READY:
		//do not update ErrorData if a previous request has not been completed
		if(request_code != SPDM_RESPOND_IF_READY) {
			spdm_context->cache_spdm_request_size =
				spdm_context->last_spdm_request_size;
			copy_mem(spdm_context->cache_spdm_request,
				spdm_context->last_spdm_request,
				spdm_context->last_spdm_request_size);
			spdm_context->error_data.rd_exponent = 1;
			spdm_context->error_data.rd_tm = 1;
			spdm_context->error_data.request_code = request_code;
			spdm_context->error_data.token = spdm_context->current_token++;
		}
		libspdm_generate_extended_error_response(
			spdm_context, SPDM_ERROR_CODE_RESPONSE_NOT_READY, 0,
			sizeof(spdm_error_data_response_not_ready_t),
			(uint8 *)(void *)&spdm_context->error_data,
			response_size, response);
		// NOTE: Need to reset status to Normal in up level
		return RETURN_SUCCESS;
	case SPDM_RESPONSE_STATE_PROCESSING_ENCAP:
		libspdm_generate_error_response(spdm_context,
					     SPDM_ERROR_CODE_REQUEST_IN_FLIGHT,
					     0, response_size, response);
		// NOTE: Need let SPDM_ENCAPSULATED_RESPONSE_ACK reset the State
		return RETURN_SUCCESS;
	default:
		return RETURN_SUCCESS;
	}
}
