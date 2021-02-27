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

#pragma once

//! STD
#include <unordered_set>

//! QT
#include <QAbstractListModel>
#include <QJsonObject>
#include <QVariant>
#include <QVector>

//! Deps
#include <antara/gaming/ecs/system.manager.hpp>

//! Project
#include "atomicdex/api/mm2/mm2.hpp"
#include "atomicdex/data/dex/orders.and.swaps.data.hpp"
#include "atomicdex/events/events.hpp"
#include "atomicdex/models/qt.orders.proxy.model.hpp"

namespace atomic_dex
{
    class orders_model final : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(orders_proxy_model* orders_proxy_mdl READ get_orders_proxy_mdl NOTIFY ordersProxyChanged);
        Q_PROPERTY(int length READ get_length NOTIFY lengthChanged);
        Q_PROPERTY(QVariant average_events_time_registry READ get_average_events_time_registry NOTIFY onAverageEventsTimeRegistryChanged)
        Q_PROPERTY(bool fetching_busy READ is_fetching_busy WRITE set_fetching_busy NOTIFY fetchingStatusChanged)
        Q_PROPERTY(int current_page READ get_current_page WRITE set_current_page NOTIFY currentPageChanged)
        Q_PROPERTY(int limit_nb_elements READ get_limit_nb_elements WRITE set_limit_nb_elements NOTIFY limitNbElementsChanged)
        Q_PROPERTY(int nb_pages READ get_nb_pages NOTIFY nbPageChanged)
        Q_ENUMS(OrdersRoles)
      public:
        enum OrdersRoles
        {
            BaseCoinRole = Qt::UserRole + 1,
            RelCoinRole,
            TickerPairRole,
            BaseCoinAmountRole,
            BaseCoinAmountCurrentCurrencyRole,
            RelCoinAmountRole,
            RelCoinAmountCurrentCurrencyRole,
            OrderTypeRole,
            IsMakerRole,
            HumanDateRole,
            UnixTimestampRole,
            OrderIdRole,
            OrderStatusRole,
            MakerPaymentIdRole,
            TakerPaymentIdRole,
            IsSwapRole,
            CancellableRole,
            IsRecoverableRole,
            OrderErrorStateRole,
            OrderErrorMessageRole,
            EventsRole,
            SuccessEventsRole,
            ErrorEventsRole
        };

        //! Constructor / destructor
        orders_model(ag::ecs::system_manager& system_manager, entt::dispatcher& dispatcher, QObject* parent = nullptr) noexcept;
        ~orders_model() noexcept final = default;

        //! Official override from Qt Model
        int                    rowCount(const QModelIndex& parent = QModelIndex()) const final;
        QVariant               data(const QModelIndex& index, int role) const final;
        bool                   removeRows(int row, int count, const QModelIndex& parent) final;
        QHash<int, QByteArray> roleNames() const final;
        bool                   setData(const QModelIndex& index, const QVariant& value, int role) final;

        //! Public api
        void refresh_or_insert(bool after_manual_reset = false);
        void reset() noexcept;
        void reset_backend() noexcept;
        bool swap_is_in_progress(const QString& coin) const noexcept;

        //! Properties
        [[nodiscard]] int                 get_length() const noexcept;
        [[nodiscard]] orders_proxy_model* get_orders_proxy_mdl() const noexcept;
        [[nodiscard]] QVariant            get_average_events_time_registry() const noexcept;
        [[nodiscard]] int                 get_current_page() const noexcept;
        void                              set_current_page(int current_page) noexcept;
        [[nodiscard]] int                 get_limit_nb_elements() const noexcept;
        void                              set_limit_nb_elements(int limit) noexcept;
        [[nodiscard]] bool                is_fetching_busy() const noexcept;
        void                              set_fetching_busy(bool fetching_status) noexcept;
        [[nodiscard]] int                 get_nb_pages() const noexcept;

        //! getter
        [[nodiscard]] t_filtering_infos get_filtering_infos() const noexcept;
        void                            set_filtering_infos(t_filtering_infos infos) noexcept;


      signals:
        void lengthChanged();
        void ordersProxyChanged();
        void onAverageEventsTimeRegistryChanged();
        void fetchingStatusChanged();
        void currentPageChanged();
        void limitNbElementsChanged();
        void nbPageChanged();

      private:
        void set_average_events_time_registry(const QVariant& average_time_registry) noexcept;
        void common_insert(const std::vector<t_order_swaps_data>& contents, const std::string& kind);

        ag::ecs::system_manager& m_system_manager;
        entt::dispatcher&        m_dispatcher;

        using t_orders_datas       = orders_and_swaps;
        using t_orders_id_registry = std::unordered_set<std::string>;
        using t_swaps_id_registry  = std::unordered_set<std::string>;

        t_orders_id_registry m_orders_id_registry;
        t_swaps_id_registry  m_swaps_id_registry;
        t_orders_datas       m_model_data;
        QVariant             m_json_time_registry;
        std::atomic_bool     m_fetching_busy{false};

        orders_proxy_model* m_model_proxy;

        //! Private common API
        void init_model(const orders_and_swaps& contents);
        void set_common_data(const orders_and_swaps& contents) noexcept;

        //! Private orders API
        void update_or_insert_orders(const orders_and_swaps& contents);
        void remove_orders(const t_orders_id_registry& are_present);
        void update_existing_order(const t_order_swaps_data& contents) noexcept;

        //! Private Swaps API
        void update_or_insert_swaps(const orders_and_swaps& contents);
        void update_swap(const t_order_swaps_data& contents) noexcept;

        //! Events
        void on_current_currency_changed(const current_currency_changed&) noexcept;
    };
} // namespace atomic_dex
