#pragma once

//! QT Headers
#include <QJsonObject>
#include <QObject>
#include <QVariant>

namespace atomic_dex
{
    class wallet_page final : public QObject, public ag::ecs::pre_update_system<wallet_page>
    {
        //! Q_Object definition
        Q_OBJECT

        //! Properties
        Q_PROPERTY(QString ticker READ get_current_ticker WRITE set_current_ticker NOTIFY currentTickerChanged)
        Q_PROPERTY(QVariant ticker_infos READ get_ticker_infos NOTIFY tickerInfosChanged)
        Q_PROPERTY(bool is_claiming_busy READ is_rpc_claiming_busy WRITE set_claiming_is_busy NOTIFY rpcClaimingStatusChanged)
        Q_PROPERTY(QVariant claiming_rpc_data READ get_rpc_claiming_data WRITE set_rpc_claiming_data NOTIFY claimingRpcDataChanged)
        Q_PROPERTY(bool is_broadcast_busy READ is_broadcast_busy WRITE set_broadcast_busy NOTIFY broadCastStatusChanged)
        Q_PROPERTY(QString broadcast_rpc_data READ get_rpc_broadcast_data WRITE set_rpc_broadcast_data NOTIFY broadcastDataChanged)

        using t_qt_synchronized_json   = boost::synchronized_value<QJsonObject>;
        using t_qt_synchronized_string = boost::synchronized_value<QString>;

        //! Private fields
        ag::ecs::system_manager& m_system_manager;
        std::atomic_bool         m_is_claiming_busy{false};
        std::atomic_bool         m_is_broadcast_busy{false};
        t_qt_synchronized_json   m_claiming_rpc_result;
        t_qt_synchronized_string m_broadcast_rpc_result;

      public:
        explicit wallet_page(entt::registry& registry, ag::ecs::system_manager& system_manager, QObject* parent = nullptr);
        void update() noexcept override;
        ~wallet_page() noexcept final = default;

        //! Public API
        void             refresh_ticker_infos() noexcept;
        Q_INVOKABLE void claim_rewards();
        Q_INVOKABLE void broadcast(const QString& tx_hex) noexcept;

        //! Properties
        [[nodiscard]] QString  get_current_ticker() const noexcept;
        void                   set_current_ticker(const QString& ticker) noexcept;
        [[nodiscard]] QVariant get_ticker_infos() const noexcept;
        [[nodiscard]] bool     is_broadcast_busy() const noexcept;
        void                   set_broadcast_busy(bool status) noexcept;
        [[nodiscard]] bool     is_rpc_claiming_busy() const noexcept;
        void                   set_claiming_is_busy(bool status) noexcept;
        [[nodiscard]] QVariant get_rpc_claiming_data() const noexcept;
        void                   set_rpc_claiming_data(QVariant rpc_data) noexcept;
        [[nodiscard]] QString  get_rpc_broadcast_data() const noexcept;
        void                   set_rpc_broadcast_data(QString rpc_data) noexcept;

      signals:
        void currentTickerChanged();
        void tickerInfosChanged();
        void rpcClaimingStatusChanged();
        void claimingRpcDataChanged();
        void broadCastStatusChanged();
        void broadcastDataChanged();
    };
} // namespace atomic_dex

REFL_AUTO(type(atomic_dex::wallet_page))