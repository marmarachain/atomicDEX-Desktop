/******************************************************************************
 * Copyright © 2013-2019 The Komodo Platform Developers.                      *
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

//! Qt
#include <QAbstractListModel>
#include <QObject> //! QObject
#include <QVariantList>

//! Project include
#include "atomicdex/data/wallet/qt.addressbook.contact.contents.hpp"
#include "atomicdex/managers/qt.wallet.manager.hpp"
#include "atomicdex/models/qt.addressbook.proxy.filter.model.hpp"
#include "atomicdex/models/qt.contact.model.hpp"

namespace atomic_dex
{
    class addressbook_model final : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(addressbook_proxy_model* addressbook_proxy_mdl READ get_addressbook_proxy_mdl NOTIFY addressbookProxyChanged);
        Q_ENUMS(AddressBookRoles)

      public:
        enum AddressBookRoles
        {
            SubModelRole = Qt::UserRole + 1,
        };

      public:
        explicit addressbook_model(atomic_dex::qt_wallet_manager& wallet_manager_, QObject* parent = nullptr) noexcept;
        ~addressbook_model() noexcept final;
        [[nodiscard]] QVariant               data(const QModelIndex& index, int role) const final;
        [[nodiscard]] int                    rowCount(const QModelIndex& parent = QModelIndex()) const final;
        bool                                 insertRows(int position, int rows, const QModelIndex& parent) final;
        bool                                 removeRows(int position, int rows, const QModelIndex& parent = QModelIndex()) final;
        void                                 initializeFromCfg();
        Q_INVOKABLE void                     add_contact_entry();
        Q_INVOKABLE void                     remove_at(int position);
        Q_INVOKABLE void                     cleanup();
        [[nodiscard]] QHash<int, QByteArray> roleNames() const final;

        //! Properties
        [[nodiscard]] addressbook_proxy_model* get_addressbook_proxy_mdl() const noexcept;
      signals:
        void addressbookProxyChanged();

      private:
        atomic_dex::qt_wallet_manager&       m_wallet_manager;
        atomic_dex::addressbook_proxy_model* m_addressbook_proxy;
        QVector<contact_model*>              m_addressbook;
        bool                                 m_should_delete_contacts{false};
    };
} // namespace atomic_dex
