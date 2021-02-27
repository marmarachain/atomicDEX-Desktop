/******************************************************************************
 * Copyright © 2013-2021 The Komodo Platform Developers.                      *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Komodo Platform software, including this file may be copied, modified,     *
 * propagated or distributed except according to the terms contained in the   *
 * LICENSE file                                                               *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

//! Deps
#include <nlohmann/json.hpp>

//! Project Headers
#include "atomicdex/api/mm2/generics.hpp"
#include "atomicdex/api/mm2/rpc.max.taker.vol.hpp"
#include "atomicdex/api/mm2/rpc.trade.preimage.hpp"

namespace mm2::api
{
    template <typename RpcSuccessReturnType, typename RpcReturnType>
    void
    extract_rpc_json_answer(const nlohmann::json& j, RpcReturnType& answer)
    {
        if (j.contains("error") && j.at("error").is_string())
        {
            answer.error = j.at("error").get<std::string>();
        }
        else if (j.contains("result"))
        {
            answer.result = j.at("result").get<RpcSuccessReturnType>();
        }
    }

    template void extract_rpc_json_answer<trade_preimage_answer_success>(const nlohmann::json& j, trade_preimage_answer& answer);
    template void extract_rpc_json_answer<max_taker_vol_answer_success>(const nlohmann::json& j, max_taker_vol_answer& answer);
} // namespace mm2::api