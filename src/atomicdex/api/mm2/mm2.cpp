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
#include <boost/algorithm/string/case_conv.hpp>

//! Project Headers
#include "atomicdex/api/mm2/mm2.hpp"
#include "atomicdex/api/mm2/rpc.trade.preimage.hpp"
#include "atomicdex/pages/qt.settings.page.hpp"
#include "atomicdex/services/price/global.provider.hpp"
#include "atomicdex/utilities/global.utilities.hpp"
#include "atomicdex/utilities/qt.utilities.hpp"

//! Utilities
namespace
{
    ag::ecs::system_manager* g_system_mgr = nullptr;

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

    std::pair<QString, QString>
    extract_error(const nlohmann::json& events, const QStringList& error_events)
    {
        for (auto&& cur_event: events)
        {
            if (std::any_of(std::begin(error_events), std::end(error_events), [&cur_event](auto&& error_str) {
                    return cur_event.at("state").get<std::string>() == error_str.toStdString();
                }))
            {
                //! It's an error
                if (cur_event.contains("data") && cur_event.at("data").contains("error"))
                {
                    return {
                        QString::fromStdString(cur_event.at("state").get<std::string>()),
                        QString::fromStdString(cur_event.at("data").at("error").get<std::string>())};
                }
            }
        }
        return {};
    }

    QString
    determine_order_status_from_last_event(const nlohmann::json& events, const QStringList& error_events) noexcept
    {
        if (events.empty())
        {
            return "matching";
        }
        auto last_event = events.back().at("state").get<std::string>();
        if (last_event == "Started")
        {
            return "matched";
        }

        if (last_event == "TakerPaymentWaitRefundStarted" || last_event == "MakerPaymentWaitRefundStarted")
        {
            return "refunding";
        }

        QString status = "ongoing";

        if (last_event == "Finished")
        {
            status = "successful";
            //! Find error or not
            for (auto&& cur_event: events)
            {
                if (cur_event.contains("data") && cur_event.at("data").contains("error") &&
                    std::any_of(std::begin(error_events), std::end(error_events), [&cur_event](auto&& error_str) {
                        return cur_event.at("state").get<std::string>() == error_str.toStdString();
                    }))
                {
                    status = "failed";
                }
            }
        }

        return status;
    }

    QString
    determine_payment_id(const nlohmann::json& events, bool am_i_maker, bool want_taker_id) noexcept
    {
        QString result = "";

        if (events.empty())
        {
            return result;
        }

        std::string search_name;
        if (am_i_maker)
        {
            search_name = want_taker_id ? "TakerPaymentSpent" : "MakerPaymentSent";
        }
        else
        {
            search_name = want_taker_id ? "TakerPaymentSent" : "MakerPaymentSpent";
        }
        for (auto&& cur_event: events)
        {
            if (cur_event.at("state").get<std::string>() == search_name)
            {
                result = QString::fromStdString(cur_event.at("data").at("tx_hash").get<std::string>());
            }
        }
        return result;
    }

    std::pair<std::string, std::string>
    determine_amounts_in_current_currency(
        const std::string& base_coin, const std::string& base_amount, const std::string& rel_coin, const std::string& rel_amount) noexcept
    {
        try
        {
            if (g_system_mgr != nullptr && g_system_mgr->has_systems<atomic_dex::settings_page, atomic_dex::global_price_service>())
            {
                const auto& settings_system     = g_system_mgr->get_system<atomic_dex::settings_page>();
                const auto& current_currency    = settings_system.get_current_currency().toStdString();
                const auto& global_price_system = g_system_mgr->get_system<atomic_dex::global_price_service>();
                std::string base_amount_in_currency;
                std::string rel_amount_in_currency;

                base_amount_in_currency = global_price_system.get_price_as_currency_from_amount(current_currency, base_coin, base_amount);
                rel_amount_in_currency  = global_price_system.get_price_as_currency_from_amount(current_currency, rel_coin, rel_amount);
                return std::make_pair(base_amount_in_currency, rel_amount_in_currency);
            }
        }
        catch (const std::exception& error)
        {
            SPDLOG_ERROR("Exception caught: {}", error.what());
        }
        return {};
    }
} // namespace

//! Implementation RPC [disable_coin]
namespace mm2::api
{
    //! Serialization
    void
    to_json(nlohmann::json& j, const disable_coin_request& req)
    {
        j["coin"] = req.coin;
    }

    //! Deserialization
    void
    from_json(const nlohmann::json& j, disable_coin_answer_success& resp)
    {
        j.at("coin").get_to(resp.coin);
    }

    void
    from_json(const nlohmann::json& j, disable_coin_answer& resp)
    {
        extract_rpc_json_answer<disable_coin_answer_success>(j, resp);
    }
} // namespace mm2::api

namespace mm2::api
{
    void
    from_json(const nlohmann::json& j, fee_regular_coin& cfg)
    {
        j.at("amount").get_to(cfg.amount);
    }

    void
    from_json(const nlohmann::json& j, fee_erc_coin& cfg)
    {
        j.at("coin").get_to(cfg.coin);
        j.at("gas").get_to(cfg.gas);
        j.at("gas_price").get_to(cfg.gas_price);
        j.at("total_fee").get_to(cfg.total_fee);
    }

    void
    from_json(const nlohmann::json& j, fee_qrc_coin& cfg)
    {
        j.at("coin").get_to(cfg.coin);
        j.at("gas_limit").get_to(cfg.gas_limit);
        j.at("gas_price").get_to(cfg.gas_price);
        j.at("miner_fee").get_to(cfg.miner_fee);
        j.at("total_gas_fee").get_to(cfg.total_gas_fee);
    }

    void
    from_json(const nlohmann::json& j, fees_data& cfg)
    {
        if (j.count("amount") == 1)
        {
            cfg.normal_fees = fee_regular_coin{};
            from_json(j, cfg.normal_fees.value());
        }
        else if (j.at("coin").get<std::string>() == "ETH")
        {
            cfg.erc_fees = fee_erc_coin{};
            from_json(j, cfg.erc_fees.value());
        }
        else if (j.at("coin").get<std::string>() == "QTUM" || j.at("coin").get<std::string>() == "tQTUM")
        {
            cfg.qrc_fees = fee_qrc_coin{};
            from_json(j, cfg.qrc_fees.value());
        }
    }

    void
    to_json(nlohmann::json& j, const tx_history_request& cfg)
    {
        j["coin"]  = cfg.coin;
        j["limit"] = cfg.limit;
    }

    void
    from_json(const nlohmann::json& j, transaction_data& cfg)
    {
        j.at("block_height").get_to(cfg.block_height);
        j.at("coin").get_to(cfg.coin);
        if (j.count("confirmations") == 1)
        {
            cfg.confirmations = j.at("confirmations").get<std::size_t>();
        }
        j.at("fee_details").get_to(cfg.fee_details);
        j.at("from").get_to(cfg.from);
        j.at("internal_id").get_to(cfg.internal_id);
        j.at("my_balance_change").get_to(cfg.my_balance_change);
        j.at("received_by_me").get_to(cfg.received_by_me);
        j.at("spent_by_me").get_to(cfg.spent_by_me);
        j.at("timestamp").get_to(cfg.timestamp);
        j.at("to").get_to(cfg.to);
        j.at("total_amount").get_to(cfg.total_amount);
        j.at("tx_hash").get_to(cfg.tx_hash);
        j.at("tx_hex").get_to(cfg.tx_hex);

        std::string s         = atomic_dex::utils::to_human_date<std::chrono::seconds>(cfg.timestamp, "%e %b %Y, %H:%M");
        cfg.timestamp_as_date = std::move(s);
    }

    void
    from_json(const nlohmann::json& j, sync_status_additional_error& answer)
    {
        j.at("code").get_to(answer.code);
        j.at("message").get_to(answer.message);
    }


    void
    from_json(const nlohmann::json& j, sync_status_eth_erc_20_coins& answer)
    {
        j.at("blocks_left").get_to(answer.blocks_left);
    }

    void
    from_json(const nlohmann::json& j, sync_status_regular_coins& answer)
    {
        j.at("transactions_left").get_to(answer.transactions_left);
    }

    void
    from_json(const nlohmann::json& j, sync_status_additional_infos& answer)
    {
        if (j.count("error") == 1)
        {
            answer.error = j.get<sync_status_additional_error>();
        }
        else if (j.count("blocks_left") == 1)
        {
            answer.erc_infos = j.get<sync_status_eth_erc_20_coins>();
        }
        else if (j.count("transactions_left") == 1)
        {
            answer.regular_infos = j.get<sync_status_regular_coins>();
        }
    }

    void
    from_json(const nlohmann::json& j, t_sync_status& answer)
    {
        j.at("state").get_to(answer.state);
        if (j.count("additional_info") == 1)
        {
            answer.additional_info = j.at("additional_info").get<sync_status_additional_infos>();
        }
    }

    void
    from_json(const nlohmann::json& j, tx_history_answer_success& answer)
    {
        if (j.contains("from_id"))
        {
            if (not j.at("from_id").is_null())
                j.at("from_id").get_to(answer.from_id);
        }
        if (j.contains("current_block"))
        {
            j.at("current_block").get_to(answer.current_block);
        }
        j.at("limit").get_to(answer.limit);
        j.at("skipped").get_to(answer.skipped);
        if (j.contains("sync_status"))
        {
            j.at("sync_status").get_to(answer.sync_status);
        }
        j.at("total").get_to(answer.total);
        j.at("transactions").get_to(answer.transactions);
    }

    void
    from_json(const nlohmann::json& j, tx_history_answer& answer)
    {
        if (j.count("error") == 1)
        {
            answer.error = j.at("error").get<std::string>();
        }
        else
        {
            answer.result = j.at("result").get<tx_history_answer_success>();
        }
    }

    void
    to_json(nlohmann::json& j, const recover_funds_of_swap_request& cfg)
    {
        j["params"]         = nlohmann::json::object();
        j["params"]["uuid"] = cfg.swap_uuid;
    }

    void
    from_json(const nlohmann::json& j, recover_funds_of_swap_answer_success& answer)
    {
        j.at("action").get_to(answer.action);
        j.at("coin").get_to(answer.coin);
        j.at("tx_hash").get_to(answer.tx_hash);
        j.at("tx_hex").get_to(answer.tx_hex);
    }

    void
    from_json(const nlohmann::json& j, recover_funds_of_swap_answer& answer)
    {
        if (j.count("error") == 1)
        {
            answer.error = j.at("error").get<std::string>();
        }
        else
        {
            answer.result = j.at("result").get<recover_funds_of_swap_answer_success>();
        }
    }

    void
    to_json(nlohmann::json& j, const withdraw_fees& cfg)
    {
        j["type"] = cfg.type;
        if (cfg.type == "EthGas")
        {
            j["gas"]       = cfg.gas_limit.value_or(55000);
            j["gas_price"] = cfg.gas_price.value();
        }
        else if (cfg.type == "Qrc20Gas")
        {
            j["gas_limit"] = cfg.gas_limit.value_or(40);
            j["gas_price"] = std::stoi(cfg.gas_price.value());
        }
        else
        {
            j["amount"] = cfg.amount.value();
        }
    }

    void
    to_json(nlohmann::json& j, const withdraw_request& cfg)
    {
        j["coin"]   = cfg.coin;
        j["amount"] = cfg.amount;
        j["to"]     = cfg.to;
        j["max"]    = cfg.max;
        if (cfg.fees.has_value())
        {
            j["fee"] = cfg.fees.value();
        }
    }

    void
    from_json(const nlohmann::json& j, withdraw_answer& answer)
    {
        if (j.count("error") >= 1)
        {
            answer.error = j.at("error").get<std::string>();
        }
        else
        {
            answer.result = j.get<transaction_data>();
        }
    }

    void
    to_json(nlohmann::json& j, const send_raw_transaction_request& cfg)
    {
        j["coin"]   = cfg.coin;
        j["tx_hex"] = cfg.tx_hex;
    }

    void
    from_json(const nlohmann::json& j, send_raw_transaction_answer& answer)
    {
        j.at("tx_hash").get_to(answer.tx_hash);
    }

    void
    to_json(nlohmann::json& j, const orderbook_request& request)
    {
        j["base"] = request.base;
        j["rel"]  = request.rel;
    }

    void
    to_json(nlohmann::json& j, const trade_fee_request& cfg)
    {
        j["coin"] = cfg.coin;
    }

    void
    from_json(const nlohmann::json& j, order_contents& contents)
    {
        j.at("coin").get_to(contents.coin);
        j.at("address").get_to(contents.address);
        j.at("price").get_to(contents.price);
        // contents.price = t_float_50(contents.price).str(8, std::ios_base::fixed);
        j.at("price_fraction").at("numer").get_to(contents.price_fraction_numer);
        j.at("price_fraction").at("denom").get_to(contents.price_fraction_denom);
        j.at("max_volume_fraction").at("numer").get_to(contents.max_volume_fraction_numer);
        j.at("max_volume_fraction").at("denom").get_to(contents.max_volume_fraction_denom);
        j.at("maxvolume").get_to(contents.maxvolume);
        j.at("pubkey").get_to(contents.pubkey);
        j.at("age").get_to(contents.age);
        j.at("zcredits").get_to(contents.zcredits);
        j.at("uuid").get_to(contents.uuid);
        j.at("is_mine").get_to(contents.is_mine);

        if (contents.price.find('.') != std::string::npos)
        {
            boost::trim_right_if(contents.price, boost::is_any_of("0"));
            contents.price = contents.price;
        }
        contents.maxvolume = atomic_dex::utils::adjust_precision(contents.maxvolume);
        t_float_50 total_f = t_float_50(contents.price) * t_float_50(contents.maxvolume);
        contents.total     = atomic_dex::utils::adjust_precision(total_f.str());
    }

    void
    from_json(const nlohmann::json& j, orderbook_answer& answer)
    {
        using namespace date;

        j.at("base").get_to(answer.base);
        j.at("rel").get_to(answer.rel);
        j.at("askdepth").get_to(answer.askdepth);
        j.at("biddepth").get_to(answer.biddepth);
        j.at("bids").get_to(answer.bids);
        j.at("asks").get_to(answer.asks);
        j.at("numasks").get_to(answer.numasks);
        j.at("numbids").get_to(answer.numbids);
        j.at("netid").get_to(answer.netid);
        j.at("timestamp").get_to(answer.timestamp);

        answer.human_timestamp = atomic_dex::utils::to_human_date(answer.timestamp, "%Y-%m-%d %I:%M:%S");

        t_float_50 result_asks_f("0");
        for (auto&& cur_asks: answer.asks) { result_asks_f = result_asks_f + t_float_50(cur_asks.maxvolume); }

        answer.asks_total_volume = result_asks_f.str();

        t_float_50 result_bids_f("0");
        for (auto& cur_bids: answer.bids)
        {
            cur_bids.total        = cur_bids.maxvolume;
            t_float_50 new_volume = t_float_50(cur_bids.maxvolume) / t_float_50(cur_bids.price);
            cur_bids.maxvolume    = atomic_dex::utils::adjust_precision(new_volume.str());
            result_bids_f         = result_bids_f + t_float_50(cur_bids.maxvolume);
        }

        answer.bids_total_volume = result_bids_f.str();
        for (auto&& cur_asks: answer.asks)
        {
            t_float_50 percent_f   = t_float_50(cur_asks.maxvolume) / result_asks_f;
            cur_asks.depth_percent = atomic_dex::utils::adjust_precision(percent_f.str());
        }

        for (auto&& cur_bids: answer.bids)
        {
            t_float_50 percent_f   = t_float_50(cur_bids.maxvolume) / result_bids_f;
            cur_bids.depth_percent = atomic_dex::utils::adjust_precision(percent_f.str());
        }
    }

    void
    from_json(const nlohmann::json& j, trade_fee_answer& cfg)
    {
        j.at("result").at("amount").get_to(cfg.amount);
        j.at("result").at("coin").get_to(cfg.coin);
    }

    void
    from_json(const nlohmann::json& j, trading_order_contents& contents)
    {
        j.at("base").get_to(contents.base);
        j.at("base_amount").get_to(contents.base_amount);
        j.at("rel").get_to(contents.rel);
        j.at("rel_amount").get_to(contents.rel_amount);
        j.at("method").get_to(contents.method);
        j.at("action").get_to(contents.action);
        j.at("uuid").get_to(contents.uuid);
        j.at("sender_pubkey").get_to(contents.sender_pubkey);
        j.at("dest_pub_key").get_to(contents.dest_pub_key);
    }

    void
    from_json(const nlohmann::json& j, buy_answer_success& contents)
    {
        j.get_to(contents.contents);
    }

    void
    from_json(const nlohmann::json& j, buy_answer& answer)
    {
        if (j.count("error") == 1)
        {
            answer.error = j.at("error").get<std::string>();
        }
        else
        {
            answer.result = j.at("result").get<buy_answer_success>();
        }
    }

    void
    to_json(nlohmann::json& j, const buy_request& request)
    {
        j["base"]   = request.base;
        j["price"]  = request.price;
        j["rel"]    = request.rel;
        j["volume"] = request.volume;
        if (request.base_nota.has_value())
        {
            j["base_nota"] = request.base_nota.value();
        }
        if (request.base_confs.has_value())
        {
            j["base_confs"] = request.base_confs.value();
        }
        if (not request.is_created_order)
        {
            //! From orderbook
            nlohmann::json price_fraction_repr  = nlohmann::json::object();
            price_fraction_repr["numer"]        = request.price_numer;
            price_fraction_repr["denom"]        = request.price_denom;
            j["price"]                          = price_fraction_repr;
            nlohmann::json volume_fraction_repr = nlohmann::json::object();
            if (not request.selected_order_use_input_volume)
            {
                volume_fraction_repr["numer"] = request.volume_numer;
                volume_fraction_repr["denom"] = request.volume_denom;
                j["volume"]                   = volume_fraction_repr;
            }
            SPDLOG_INFO("The order is picked from the orderbook price: {}, volume: {}", j.at("price").dump(4), j.at("volume").dump(4));
        }
        else
        {
            SPDLOG_INFO("The order is not picked from orderbook we create it from volume = {}, price = {}", j.at("volume").dump(4), request.price);
        }
    }

    void
    to_json(nlohmann::json& j, const setprice_request& request)
    {
        j["base"]            = request.base;
        j["price"]           = request.price;
        j["rel"]             = request.rel;
        j["volume"]          = request.volume;
        j["cancel_previous"] = request.cancel_previous;
        j["max"]             = request.max;
        if (request.base_nota.has_value())
        {
            j["base_nota"] = request.base_nota.value();
        }
        if (request.base_confs.has_value())
        {
            j["base_confs"] = request.base_confs.value();
        }
        if (request.rel_nota.has_value())
        {
            j["rel_nota"] = request.rel_nota.value();
        }
        if (request.rel_confs.has_value())
        {
            j["rel_confs"] = request.rel_confs.value();
        }
    }

    void
    to_json(nlohmann::json& j, const sell_request& request)
    {
        SPDLOG_DEBUG("price: {}, volume: {}", request.price, request.volume);

        auto volume_fraction_functor = [&request]() {
            nlohmann::json volume_fraction_repr = nlohmann::json::object();
            volume_fraction_repr["numer"]       = request.volume_numer;
            volume_fraction_repr["denom"]       = request.volume_denom;
            return volume_fraction_repr;
        };

        j["base"]   = request.base;
        j["rel"]    = request.rel;
        j["volume"] = request.volume; //< First take the user input
        if (request.is_max)           //< It's a real max means user want to sell his base_max_taker_vol let's take the fraction repr
        {
            j["volume"] = volume_fraction_functor();
        }
        j["price"] = request.price;
        if (request.rel_nota.has_value())
        {
            j["rel_nota"] = request.rel_nota.value();
        }
        if (request.rel_confs.has_value())
        {
            j["rel_confs"] = request.rel_confs.value();
        }

        if (not request.is_created_order)
        {
            //! From orderbook
            nlohmann::json price_fraction_repr = nlohmann::json::object();
            price_fraction_repr["numer"]       = request.price_numer;
            price_fraction_repr["denom"]       = request.price_denom;
            j["price"]                         = price_fraction_repr;
            if (request.is_exact_selected_order_volume)
            {
                j["volume"] = volume_fraction_functor();
            }
            SPDLOG_INFO("The order is picked from the orderbook price: {}, volume: {}", j.at("price").dump(4), j.at("volume").dump(4));
        }
        else
        {
            SPDLOG_INFO("The order is not picked from orderbook we create it from volume = {}, price = {}", j.at("volume").dump(4), request.price);
        }
    }

    void
    from_json(const nlohmann::json& j, sell_answer_success& contents)
    {
        j.get_to(contents.contents);
    }

    void
    from_json(const nlohmann::json& j, sell_answer& answer)
    {
        if (j.count("error") == 1)
        {
            answer.error = j.at("error").get<std::string>();
        }
        else
        {
            answer.result = j.at("result").get<sell_answer_success>();
        }
    }

    void
    to_json(nlohmann::json& j, const cancel_order_request& request)
    {
        j["uuid"] = request.uuid;
    }

    void
    from_json(const nlohmann::json& j, cancel_order_answer& answer)
    {
        if (j.count("error") == 1)
        {
            answer.error = j.at("error").get<std::string>();
        }
        else
        {
            answer.result = j.at("result").get<std::string>();
        }
    }

    void
    to_json(nlohmann::json& j, const cancel_data& cfg)
    {
        if (cfg.pair.has_value())
        {
            auto [base, rel] = cfg.pair.value();
            j["base"]        = base;
            j["rel"]         = rel;
        }
        else if (cfg.ticker.has_value())
        {
            j["ticker"] = cfg.ticker.value();
        }
    }

    void
    to_json(nlohmann::json& j, const cancel_type& cfg)
    {
        j["type"] = cfg.type;
        if (cfg.type not_eq "All" and cfg.data.has_value())
        {
            j["data"] = cfg.data.value();
        }
    }

    void
    to_json(nlohmann::json& j, const cancel_all_orders_request& cfg)
    {
        j["cancel_by"] = cfg.cancel_by;
    }

    void
    from_json(const nlohmann::json& j, cancel_all_orders_answer& answer)
    {
        j.at("result").at("cancelled").get_to(answer.cancelled);
        j.at("result").at("currently_matching").get_to(answer.currently_matching);
    }

    void
    from_json(const nlohmann::json& j, my_orders_answer& answer)
    {
        // answer.orders.reserve(j.at("result").at("maker_orders").size() + j.at("result").at("taker_orders").size());

        auto filler_functor = [&answer](const std::string& key, const nlohmann::json& value, bool is_maker) {
            using namespace date;
            const auto time_key = value.at("created_at").get<std::size_t>();

            std::string action = "";
            if (not is_maker)
            {
                value.at("request").at("action").get_to(action);
            }
            using namespace atomic_dex;
            const auto price       = is_maker ? atomic_dex::utils::adjust_precision(value.at("price").get<std::string>()) : "0";
            const auto base_coin   = is_maker ? QString::fromStdString(value.at("base").get<std::string>())
                                              : QString::fromStdString(value.at("request").at("base").get<std::string>());
            const auto rel_coin    = is_maker ? QString::fromStdString(value.at("rel").get<std::string>())
                                              : QString::fromStdString(value.at("request").at("rel").get<std::string>());
            const auto base_amount = is_maker ? QString::fromStdString(value.at("available_amount").get<std::string>())
                                              : QString::fromStdString(value.at("request").at("base_amount").get<std::string>());
            const auto rel_amount  = is_maker ? QString::fromStdString((t_float_50(price) * t_float_50(base_amount.toStdString())).convert_to<std::string>())
                                              : QString::fromStdString(value.at("request").at("rel_amount").get<std::string>());
            order_swaps_data contents{
                .is_maker       = is_maker,
                .base_coin      = action == "Sell" ? base_coin : rel_coin,
                .rel_coin       = action == "Sell" ? rel_coin : base_coin,
                .base_amount    = action == "Sell" ? base_amount : rel_amount,
                .rel_amount     = action == "Sell" ? rel_amount : base_amount,
                .order_type     = is_maker ? "maker" : "taker",
                .human_date     = QString::fromStdString(atomic_dex::utils::to_human_date<std::chrono::seconds>(time_key / 1000, "%F    %T")),
                .unix_timestamp = static_cast<unsigned long long>(time_key),
                .order_id       = QString::fromStdString(key),
                .order_status   = "matching",
                .is_swap        = false,
                .is_cancellable = value.at("cancellable").get<bool>(),
                .is_recoverable = false};
            if (action.empty() && contents.order_type == "maker")
            {
                contents.base_coin   = base_coin;
                contents.rel_coin    = rel_coin;
                contents.base_amount = base_amount;
                contents.rel_amount  = rel_amount;
            }
            auto&& [base_fiat_value, rel_fiat_value] = determine_amounts_in_current_currency(
                contents.base_coin.toStdString(), contents.base_amount.toStdString(), contents.rel_coin.toStdString(), contents.rel_amount.toStdString());
            contents.base_amount_fiat = QString::fromStdString(base_fiat_value);
            contents.rel_amount_fiat  = QString::fromStdString(rel_fiat_value);
            contents.ticker_pair      = contents.base_coin + "/" + contents.rel_coin;
            answer.orders_id.emplace(key);
            answer.orders.emplace_back(std::move(contents));
        };

        for (auto&& [key, value]: j.at("result").at("maker_orders").items()) { filler_functor(key, value, true); }
        for (auto&& [key, value]: j.at("result").at("taker_orders").items()) { filler_functor(key, value, false); }
    }

    void
    to_json(nlohmann::json& j, const active_swaps_request& request)
    {
        if (request.statuses.has_value())
        {
            j["statuses"] = request.statuses.value();
        }
        else
        {
            j["statuses"] = nullptr;
        }
    }

    void
    from_json(const nlohmann::json& j, active_swaps_answer& answer)
    {
        if (j.contains("statuses"))
        {
            const auto& statuses = j.at("statuses");
            j.at("uuids").get_to(answer.uuids);
            answer.swaps.reserve(answer.uuids.size());
            for (auto&& [key, value]: statuses.items())
            {
                order_swaps_data to_add;
                from_json(value, to_add);
                answer.swaps.emplace_back(std::move(to_add));
            }
        }
    }

    void
    to_json(nlohmann::json& j, const show_priv_key_request& request)
    {
        j["coin"] = request.coin;
    }

    void
    from_json(const nlohmann::json& j, show_priv_key_answer& answer)
    {
        j.at("result").at("coin").get_to(answer.coin);
        j.at("result").at("priv_key").get_to(answer.priv_key);
    }

    void
    to_json(nlohmann::json& j, const my_recent_swaps_request& request)
    {
        j["limit"] = request.limit;
        if (request.from_uuid.has_value())
        {
            j["from_uuid"] = request.from_uuid.value();
        }
        if (request.page_number.has_value())
        {
            j["page_number"] = request.page_number.value();
        }

        if (request.my_coin.has_value())
        {
            j["my_coin"] = request.my_coin.value();
        }

        if (request.other_coin.has_value())
        {
            j["other_coin"] = request.other_coin.value();
        }

        if (request.from_timestamp.has_value())
        {
            j["from_timestamp"] = request.from_timestamp.value();
        }

        if (request.to_timestamp.has_value())
        {
            j["to_timestamp"] = request.to_timestamp.value();
        }
        // SPDLOG_INFO("Full request: {}", j.dump(4));
    }

    void
    from_json(const nlohmann::json& j, order_swaps_data& contents)
    {
        // spdlog::stopwatch stopwatch;
        using namespace date;
        using namespace std::chrono;
        using namespace atomic_dex;

        const auto taker_coin   = QString::fromStdString(j.at("taker_coin").get<std::string>());
        const auto maker_coin   = QString::fromStdString(j.at("maker_coin").get<std::string>());
        const auto maker_amount = QString::fromStdString(atomic_dex::utils::adjust_precision(j.at("maker_amount").get<std::string>()));
        const auto taker_amount = QString::fromStdString(atomic_dex::utils::adjust_precision(j.at("taker_amount").get<std::string>()));

        contents.error_events   = vector_std_string_to_qt_string_list(j.at("error_events").get<std::vector<std::string>>());
        contents.success_events = vector_std_string_to_qt_string_list(j.at("success_events").get<std::vector<std::string>>());
        contents.order_id       = QString::fromStdString(j.at("uuid").get<std::string>());
        contents.order_type     = QString::fromStdString(boost::algorithm::to_lower_copy(j.at("type").get<std::string>()));
        contents.is_maker       = contents.order_type == "maker";
        contents.is_recoverable = j.at("recoverable").get<bool>();
        contents.base_coin      = contents.is_maker ? maker_coin : taker_coin;
        contents.rel_coin       = contents.is_maker ? taker_coin : maker_coin;
        contents.base_amount    = contents.is_maker ? maker_amount : taker_amount;
        contents.rel_amount     = contents.is_maker ? taker_amount : maker_amount;

        nlohmann::json events_array = nlohmann::json::array();

        using t_event_timestamp_registry = std::unordered_map<std::string, std::uint64_t>;
        t_event_timestamp_registry event_timestamp_registry;
        double                     total_time_in_ms = 0.00;

        std::size_t idx    = 0;
        const auto& events = j.at("events");
        for (auto&& content: events)
        {
            const nlohmann::json& j_evt      = content.at("event");
            auto                  timestamp  = content.at("timestamp").get<std::size_t>();
            std::string           human_date = atomic_dex::utils::to_human_date<std::chrono::seconds>(timestamp / 1000, "%F    %H:%M:%S");
            auto                  evt_type   = j_evt.at("type").get<std::string>();

            auto rate_bundler = [&event_timestamp_registry,
                                 &total_time_in_ms](nlohmann::json& jf_evt, const std::string& event_type, const std::string& previous_event) {
                if (event_timestamp_registry.count(previous_event) != 0)
                {
                    std::int64_t ts                               = event_timestamp_registry.at(previous_event);
                    jf_evt["started_at"]                          = ts;
                    std::int64_t                              ts2 = jf_evt.at("timestamp").get<std::int64_t>();
                    date::sys_time<std::chrono::milliseconds> t1{std::chrono::milliseconds{ts}};
                    date::sys_time<std::chrono::milliseconds> t2{std::chrono::milliseconds{ts2}};
                    double                                    res = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
                    jf_evt["time_diff"]                           = res;
                    event_timestamp_registry[event_type]          = ts2; // Negotiated finished at this time
                    total_time_in_ms += res;
                }
            };

            if (j_evt.count("data") == 0)
            {
                nlohmann::json jf_evt = {{"state", evt_type}, {"human_timestamp", human_date}, {"timestamp", timestamp}};

                if (idx > 0)
                {
                    rate_bundler(jf_evt, evt_type, events[idx - 1].at("event").at("type").get<std::string>());
                }

                events_array.push_back(jf_evt);
            }
            else
            {
                nlohmann::json jf_evt = {{"state", evt_type}, {"human_timestamp", human_date}, {"data", j_evt.at("data")}, {"timestamp", timestamp}};
                if (evt_type == "Started")
                {
                    std::int64_t ts                               = jf_evt.at("data").at("started_at").get<std::int64_t>() * 1000;
                    jf_evt["started_at"]                          = ts;
                    std::int64_t                              ts2 = jf_evt.at("timestamp").get<std::int64_t>();
                    date::sys_time<std::chrono::milliseconds> t1{std::chrono::milliseconds{ts}};
                    date::sys_time<std::chrono::milliseconds> t2{std::chrono::milliseconds{ts2}};
                    double                                    res = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
                    jf_evt["time_diff"]                           = res;
                    event_timestamp_registry["Started"]           = ts2; // Started finished at this time
                    total_time_in_ms += res;
                }

                if (idx > 0)
                {
                    rate_bundler(jf_evt, evt_type, events[idx - 1].at("event").at("type").get<std::string>());
                }

                events_array.push_back(jf_evt);
            }
            idx += 1;
        }
        contents.events           = nlohmann_json_array_to_qt_json_array(events_array);
        contents.human_date       = not events_array.empty() ? QString::fromStdString(events_array.back().at("human_timestamp").get<std::string>()) : "";
        contents.unix_timestamp   = not events_array.empty() ? events_array.back().at("timestamp").get<unsigned long long>() : 0;
        contents.order_status     = determine_order_status_from_last_event(events_array, contents.error_events);
        contents.is_swap          = true;
        contents.is_cancellable   = false;
        contents.maker_payment_id = determine_payment_id(events_array, contents.is_maker, false);
        contents.taker_payment_id = determine_payment_id(events_array, contents.is_maker, true);

        auto&& [base_fiat_value, rel_fiat_value] = determine_amounts_in_current_currency(
            contents.base_coin.toStdString(), contents.base_amount.toStdString(), contents.rel_coin.toStdString(), contents.rel_amount.toStdString());
        contents.base_amount_fiat = QString::fromStdString(base_fiat_value);
        contents.rel_amount_fiat  = QString::fromStdString(rel_fiat_value);
        contents.ticker_pair      = contents.base_coin + "/" + contents.rel_coin;
        if (contents.order_status == "failed")
        {
            auto error                   = extract_error(events_array, contents.error_events);
            contents.order_error_state   = error.first;
            contents.order_error_message = error.second;
        }
        // SPDLOG_INFO("from_json(order_swaps_data) -> {} seconds", stopwatch);
    }

    void
    from_json(const nlohmann::json& j, my_recent_swaps_answer_success& results)
    {
        // spdlog::stopwatch                                    stopwatch;
        std::unordered_map<std::string, std::vector<double>> events_time_registry;
        const auto&                                          swaps = j.at("swaps");
        results.swaps.reserve(swaps.size());
        results.swaps_id.reserve(swaps.size());
        for (auto&& cur: swaps)
        {
            if (cur.is_null())
            {
                SPDLOG_WARN("Current swap object is null - skipping");
                continue;
            }
            order_swaps_data to_add;
            from_json(cur, to_add);
            for (auto&& cur_event: to_add.events)
            {
                if (cur_event.isObject())
                {
                    if (auto cur_obj = cur_event.toObject(); cur_obj.contains("time_diff"))
                    {
                        events_time_registry[cur_obj.value("state").toString().toStdString()].push_back(cur_obj.value("time_diff").toDouble());
                    }
                }
            }
            results.swaps_id.emplace(to_add.order_id.toStdString());
            results.swaps.emplace_back(std::move(to_add));
        }
        j.at("limit").get_to(results.limit);
        j.at("skipped").get_to(results.skipped);
        j.at("total").get_to(results.total);
        j.at("page_number").get_to(results.page_number);
        j.at("total_pages").get_to(results.total_pages);
        results.average_events_time = nlohmann::json::object();

        for (auto&& [evt_name, values]: events_time_registry)
        {
            double sum = 0;
            for (auto&& cur_value: values) { sum += cur_value; }
            double average                        = sum / values.size();
            results.average_events_time[evt_name] = average;
        }
    }

    void
    from_json(const nlohmann::json& j, my_recent_swaps_answer& answer)
    {
        if (j.find("result") != j.end())
        {
            answer.result                    = j.at("result").get<my_recent_swaps_answer_success>();
            answer.result.value().raw_result = answer.raw_result;
        }
        else if (j.find("error") != j.end())
        {
            answer.error = j.at("error").get<std::string>();
        }
    }

    disable_coin_answer
    rpc_disable_coin(disable_coin_request&& request, std::shared_ptr<t_http_client> mm2_client)
    {
        return process_rpc<disable_coin_request, disable_coin_answer>(std::forward<disable_coin_request>(request), "disable_coin", mm2_client);
    }

    recover_funds_of_swap_answer
    rpc_recover_funds(recover_funds_of_swap_request&& request, std::shared_ptr<t_http_client> mm2_client)
    {
        return process_rpc<recover_funds_of_swap_request, recover_funds_of_swap_answer>(
            std::forward<recover_funds_of_swap_request>(request), "recover_funds_of_swap", mm2_client);
    }

    template <typename TRequest, typename TAnswer>
    TAnswer
    process_rpc(TRequest&& request, std::string rpc_command, std::shared_ptr<t_http_client> mm2_http_client)
    {
        SPDLOG_INFO("Processing rpc call: {}", rpc_command);

        nlohmann::json json_data = template_request(rpc_command);

        to_json(json_data, request);

        auto json_copy        = json_data;
        json_copy["userpass"] = "*******";
        SPDLOG_DEBUG("request: {}", json_copy.dump());

        if (mm2_http_client != nullptr)
        {
            web::http::http_request request(web::http::methods::POST);
            request.headers().set_content_type(FROM_STD_STR("application/json"));
            request.set_body(json_data.dump());
            auto resp = mm2_http_client->request(request).get();
            return rpc_process_answer<TAnswer>(resp, rpc_command);
        }

        return TAnswer{};
    }

    nlohmann::json
    template_request(std::string method_name) noexcept
    {
        return {{"method", std::move(method_name)}, {"userpass", get_rpc_password()}};
    }

    std::string
    rpc_version()
    {
        nlohmann::json json_data = template_request("version");
        try
        {
            auto                    client = std::make_unique<web::http::client::http_client>(FROM_STD_STR("http://127.0.0.1:7783"));
            web::http::http_request request;
            request.set_method(web::http::methods::POST);
            request.set_body(json_data.dump());
            web::http::http_response resp = client->request(request).get();
            if (resp.status_code() == 200)
            {
                std::string    body      = TO_STD_STR(resp.extract_string(true).get());
                nlohmann::json body_json = nlohmann::json::parse(body);
                return body_json.at("result").get<std::string>();
            }

            return "error occured during rpc_version";
        }
        catch (const web::http::http_exception& exception)
        {
            return "error occured during rpc_version";
        }
        return "";
    }

    kmd_rewards_info_answer
    process_kmd_rewards_answer(nlohmann::json result)
    {
        kmd_rewards_info_answer out;
        out.result                                       = result;
        auto transform_timestamp_into_human_date_functor = [](nlohmann::json& obj, const std::string& field) {
            if (obj.contains(field))
            {
                auto obj_timestamp         = obj.at(field).get<std::size_t>();
                obj[field + "_human_date"] = atomic_dex::utils::to_human_date<std::chrono::seconds>(obj_timestamp, "%e %b %Y, %H:%M");
            }
        };

        for (auto&& obj: out.result.at("result"))
        {
            for (const auto& field: {"accrue_start_at", "accrue_stop_at", "locktime"}) { transform_timestamp_into_human_date_functor(obj, field); }
        }
        return out;
    }

    pplx::task<web::http::http_response>
    async_rpc_batch_standalone(nlohmann::json batch_array, std::shared_ptr<t_http_client> mm2_http_client, pplx::cancellation_token token)
    {
        if (mm2_http_client != nullptr)
        {
            web::http::http_request request;
            request.set_method(web::http::methods::POST);
            request.set_body(batch_array.dump());
            auto resp = mm2_http_client->request(request, token);
            return resp;
        }
        return {};
    }

    nlohmann::json
    basic_batch_answer(const web::http::http_response& resp)
    {
        nlohmann::json answer;
        std::string    body = TO_STD_STR(resp.extract_string(true).get());
        try
        {
            answer = nlohmann::json::parse(body);
        }
        catch (const nlohmann::detail::parse_error& err)
        {
            SPDLOG_ERROR("exception caught {}, body: {}", err.what(), body);
            answer["error"] = body;
        }
        return answer;
    }

    static inline std::string&
    access_rpc_password() noexcept
    {
        static std::string rpc_password;
        return rpc_password;
    }

    void
    set_rpc_password(std::string rpc_password) noexcept
    {
        access_rpc_password() = std::move(rpc_password);
    }

    const std::string&
    get_rpc_password() noexcept
    {
        return access_rpc_password();
    }

    pplx::task<web::http::http_response>
    async_process_rpc_get(t_http_client_ptr& client, const std::string rpc_command, const std::string& url)
    {
        SPDLOG_INFO("Processing rpc call: {}, url: {}, endpoint: {}", rpc_command, url, TO_STD_STR(client->base_uri().to_string()));

        web::http::http_request req;
        req.set_method(web::http::methods::GET);
        if (not url.empty())
        {
            req.set_request_uri(FROM_STD_STR(url));
        }
        return client->request(req);
    }

    template <typename RpcReturnType>
    RpcReturnType
    rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept
    {
        RpcReturnType answer;

        try
        {
            from_json(json_answer, answer);
            answer.rpc_result_code = 200;
        }
        catch (const std::exception& error)
        {
            SPDLOG_ERROR("exception caught for rpc {} answer: {}, exception: {}", rpc_command, json_answer.dump(4), error.what());
            answer.rpc_result_code = -1;
            answer.raw_result      = error.what();
        }

        return answer;
    }

    template mm2::api::withdraw_answer        rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;
    template mm2::api::my_orders_answer       rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;
    template mm2::api::orderbook_answer       rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;
    template mm2::api::trade_fee_answer       rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;
    template mm2::api::max_taker_vol_answer   rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;
    template mm2::api::my_recent_swaps_answer rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;
    template mm2::api::active_swaps_answer    rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;
    template mm2::api::show_priv_key_answer   rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;
    template mm2::api::trade_preimage_answer  rpc_process_answer_batch(nlohmann::json& json_answer, const std::string& rpc_command) noexcept;

    template <typename RpcReturnType>
    RpcReturnType
    rpc_process_answer(const web::http::http_response& resp, const std::string& rpc_command) noexcept
    {
        std::string body = TO_STD_STR(resp.extract_string(true).get());
        SPDLOG_INFO("resp code for rpc_command {} is {}", rpc_command, resp.status_code());
        RpcReturnType answer;

        try
        {
            if (resp.status_code() not_eq 200)
            {
                SPDLOG_WARN("rpc answer code is not 200, body : {}", body);
                if constexpr (doom::meta::is_detected_v<have_error_field, RpcReturnType>)
                {
                    SPDLOG_DEBUG("error field detected inside the RpcReturnType");
                    if constexpr (std::is_same_v<std::optional<std::string>, decltype(answer.error)>)
                    {
                        SPDLOG_DEBUG("The error field type is string, parsing it from the response body");
                        if (auto json_data = nlohmann::json::parse(body); json_data.at("error").is_string())
                        {
                            answer.error = json_data.at("error").get<std::string>();
                        }
                        else
                        {
                            answer.error = body;
                        }
                        SPDLOG_DEBUG("The error after getting extracted is: {}", answer.error.value());
                    }
                }
                answer.rpc_result_code = resp.status_code();
                answer.raw_result      = body;
                return answer;
            }


            assert(not body.empty());
            auto json_answer       = nlohmann::json::parse(body);
            answer.rpc_result_code = resp.status_code();
            answer.raw_result      = body;
            from_json(json_answer, answer);
        }
        catch (const std::exception& error)
        {
            SPDLOG_ERROR(
                "{} l{} f[{}], exception caught {} for rpc {}, body: {}", __FUNCTION__, __LINE__, fs::path(__FILE__).filename().string(), error.what(),
                rpc_command, body);
            answer.rpc_result_code = -1;
            answer.raw_result      = error.what();
        }

        return answer;
    }

    void
    set_system_manager(ag::ecs::system_manager& system_manager)
    {
        g_system_mgr = std::addressof(system_manager);
    }

    template mm2::api::tx_history_answer rpc_process_answer(const web::http::http_response& resp, const std::string& rpc_command);
} // namespace mm2::api
