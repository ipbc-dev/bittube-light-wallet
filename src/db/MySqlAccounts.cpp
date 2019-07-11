//
// Created by mwo on 7/01/17.
//

#define MYSQLPP_SSQLS_NO_STATICS 1

#include "MySqlAccounts.h"
//#include "TxSearch.h"
#include "../CurrentBlockchainStatus.h"

#include "ssqlses.h"

namespace xmreg
{

bool
MysqlInputs::select_for_out(const uint64_t& output_id,
                            vector<XmrInput>& ins, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(XmrInput::SELECT_STMT4);
        query.parse();

        query.storein(ins, output_id);

        return !ins.empty();
    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}

bool
MysqlOutpus::exist(const string& output_public_key_str, XmrOutput& out, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(XmrOutput::EXIST_STMT);
        query.parse();

        vector<XmrOutput> outs;

        query.storein(outs, output_public_key_str);

        if (outs.empty())
            return false;

        out = std::move(outs.at(0));

    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return false;
    }

    return true;
}

uint64_t
MysqlTransactions::mark_spendable(const uint64_t& tx_id_no, bool spendable, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(
                    spendable ?
                      XmrTransaction::MARK_AS_SPENDABLE_STMT
                                : XmrTransaction::MARK_AS_NONSPENDABLE_STMT);
        query.parse();


        SimpleResult sr = query.execute(tx_id_no);

        return sr.rows();
    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return 0;
}

uint64_t
MysqlTransactions::delete_tx(const uint64_t& tx_id_no, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(XmrTransaction::DELETE_STMT);
        query.parse();

        SimpleResult sr = query.execute(tx_id_no);

        return sr.rows();
    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return 0;
}


bool
MysqlTransactions::exist(const uint64_t& account_id,
                         const string& tx_hash_str, XmrTransaction& tx, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(XmrTransaction::EXIST_STMT);
        query.parse();

        vector<XmrTransaction> outs;

        query.storein(outs, account_id, tx_hash_str);

        if (outs.empty())
            return false;

        tx = outs.at(0);

    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        return false;
    }

    return true;
}


bool
MysqlTransactions::get_total_recieved(const uint64_t& account_id,
                                      uint64_t& amount, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(XmrTransaction::SUM_XMR_RECIEVED);
        query.parse();

        StoreQueryResult sqr = query.store(account_id);

        if (!sqr.empty())
        {
            amount = sqr.at(0)["total_received"];
            return true;
        }
    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

bool
MysqlPayments::select_by_payment_id(const string& payment_id,
                                    vector<XmrPayment>& payments, shared_ptr<mysqlpp::Connection> conn)
{

    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(XmrPayment::SELECT_STMT2);
        query.parse();

        payments.clear();
        query.storein(payments, payment_id);

        return !payments.empty();
    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}

MySqlAccounts::MySqlAccounts(
        shared_ptr<CurrentBlockchainStatus> _current_bc_status)
    : current_bc_status {_current_bc_status}
{
    _init();
}

bool
MySqlAccounts::select(const string& address, XmrAccount& account, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(XmrAccount::SELECT_STMT2);
        query.parse();

        vector<XmrAccount> res;
        query.storein(res, address);

        if (!res.empty())
        {
            account = std::move(res.at(0));
            return true;
        }

    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}

template <typename T>
uint64_t
MySqlAccounts::insert(const T& data_to_insert, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query();

        query.insert(data_to_insert);

        SimpleResult sr = query.execute();

        if (sr.rows() == 1)
            return sr.insert_id();

    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return 0;
}

// Explicitly instantiate insert template for our tables
template
uint64_t MySqlAccounts::insert<XmrAccount>(const XmrAccount& data_to_insert, shared_ptr<mysqlpp::Connection> conn);
template
uint64_t MySqlAccounts::insert<XmrTransaction>(
                    const XmrTransaction& data_to_insert, shared_ptr<mysqlpp::Connection> conn);
template
uint64_t MySqlAccounts::insert<XmrOutput>(const XmrOutput& data_to_insert, shared_ptr<mysqlpp::Connection> conn);
template
uint64_t MySqlAccounts::insert<XmrInput>(const XmrInput& data_to_insert, shared_ptr<mysqlpp::Connection> conn);
template
uint64_t MySqlAccounts::insert<XmrPayment>(const XmrPayment& data_to_insert, shared_ptr<mysqlpp::Connection> conn);

template <typename T>
uint64_t
MySqlAccounts::insert(const vector<T>& data_to_insert, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query();

        query.insert(data_to_insert.begin(), data_to_insert.end());

        SimpleResult sr = query.execute();

        return sr.rows();

    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return 0;
}

// Explicitly instantiate insert template for our tables
template
uint64_t MySqlAccounts::insert<XmrOutput>(
        const vector<XmrOutput>& data_to_insert, shared_ptr<mysqlpp::Connection> conn);

template
uint64_t MySqlAccounts::insert<XmrInput>(
        const vector<XmrInput>& data_to_insert, shared_ptr<mysqlpp::Connection> conn);

template <typename T, size_t query_no>
bool
MySqlAccounts::select(uint64_t account_id, vector<T>& selected_data, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query((query_no == 1
                                   ? T::SELECT_STMT : T::SELECT_STMT2));
        query.parse();

        selected_data.clear();

        query.storein(selected_data, account_id);

        // this is confusing. So I get false from this method
        // when this is empty and when there is some exception!
        return !selected_data.empty();
        //return true;
    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

template
bool MySqlAccounts::select<XmrAccount>(uint64_t account_id,
        vector<XmrAccount>& selected_data, shared_ptr<mysqlpp::Connection> conn);

template
bool MySqlAccounts::select<XmrTransaction>(uint64_t account_id,
        vector<XmrTransaction>& selected_data, shared_ptr<mysqlpp::Connection> conn);

template
bool MySqlAccounts::select<XmrOutput>(uint64_t account_id,
        vector<XmrOutput>& selected_data, shared_ptr<mysqlpp::Connection> conn);

template // this will use SELECT_STMT2 which selectes
        // based on transaction id, not account_id,
bool MySqlAccounts::select<XmrOutput, 2>(uint64_t tx_id,
        vector<XmrOutput>& selected_data, shared_ptr<mysqlpp::Connection> conn);

template
bool MySqlAccounts::select<XmrInput>(uint64_t account_id,
        vector<XmrInput>& selected_data, shared_ptr<mysqlpp::Connection> conn);

template
bool MySqlAccounts::select<XmrPayment>(uint64_t account_id,
        vector<XmrPayment>& selected_data, shared_ptr<mysqlpp::Connection> conn);

template // this will use SELECT_STMT2 which selectes
         // based on transaction id, not account_id,
bool MySqlAccounts::select<XmrInput, 2>(uint64_t tx_id,
        vector<XmrInput>& selected_data, shared_ptr<mysqlpp::Connection> conn);


template <typename T>
bool
MySqlAccounts::update(T const& orginal_row, T const& new_row, shared_ptr<mysqlpp::Connection> conn)
{
    try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query();

        query.update(orginal_row, new_row);

        SimpleResult sr = query.execute();

        return sr.rows() == 1;
    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
    }

    return false;
}

template
bool MySqlAccounts::update<XmrAccount>(
        XmrAccount const& orginal_row, XmrAccount const& new_row, shared_ptr<mysqlpp::Connection> conn);

template
bool MySqlAccounts::update<XmrPayment>(
        XmrPayment const& orginal_row, XmrPayment const& new_row, shared_ptr<mysqlpp::Connection> conn);

template <typename T>
bool
MySqlAccounts::select_for_tx(uint64_t tx_id, vector<T>& selected_data, shared_ptr<mysqlpp::Connection> conn)
{
    return select<T, 2>(tx_id, selected_data, conn);
}

template // this will use SELECT_STMT2 which selectes based on
         // transaction id, not account_id,
bool MySqlAccounts::select_for_tx<XmrOutput>(uint64_t tx_id,
        vector<XmrOutput>& selected_data, shared_ptr<mysqlpp::Connection> conn);


template // this will use SELECT_STMT2 which selectes
         //based on transaction id, not account_id,
bool MySqlAccounts::select_for_tx<XmrInput>(uint64_t tx_id,
        vector<XmrInput>& selected_data, shared_ptr<mysqlpp::Connection> conn);

template <typename T>
bool
MySqlAccounts::select_by_primary_id(uint64_t id, T& selected_data, shared_ptr<mysqlpp::Connection> conn)
{
     try
    {
        // mysqlpp::ScopedConnection cp(MySqlConnectionPool::get(), true);
        if (!conn) conn = MySqlConnectionPool::get().grab_shared();
        Query query = conn->query(T::SELECT_STMT3);
        query.parse();

        vector<T> outs;

        query.storein(outs, id);

        if (!outs.empty())
        {
            selected_data = std::move(outs.at(0));
            return true;
        }
    }
    catch (std::exception const& e)
    {
        MYSQL_EXCEPTION_MSG(e);
        //throw  e;
    }

    return false;
}

//template
//bool MySqlAccounts::select_by_primary_id<XmrTransaction>(
//                           uint64_t id, XmrTransaction& selected_data);

template
bool MySqlAccounts::select_by_primary_id<XmrInput>(
        uint64_t id, XmrInput& selected_data, shared_ptr<mysqlpp::Connection> conn);

template
bool MySqlAccounts::select_by_primary_id<XmrOutput>(
        uint64_t id, XmrOutput& selected_data, shared_ptr<mysqlpp::Connection> conn);

template
bool MySqlAccounts::select_by_primary_id<XmrPayment>(
        uint64_t id, XmrPayment& selected_data, shared_ptr<mysqlpp::Connection> conn);

bool
MySqlAccounts::select_txs_for_account_spendability_check(
        const uint64_t& account_id, vector<XmrTransaction>& txs, shared_ptr<mysqlpp::Connection> conn)
{

    for (auto it = txs.begin(); it != txs.end(); )
    {
        // first we check if txs stored in db are already spendable
        // it means if they are older than 10 blocks. If  yes,
        // we mark them as spendable, as we assume that blocks
        // older than 10 blocks are permanent, i.e, they wont get
        // orphaned.

        XmrTransaction& tx = *it;

        if (bool {tx.spendable} == false)
        {

            if (current_bc_status->is_tx_unlocked(tx.unlock_time, tx.height))
            {
                // this tx was before marked as unspendable, but now
                // it is spendable. Meaning, that its older than 10 blocks.
                // so mark it as spendable in mysql, so that its permanet.

                uint64_t no_row_updated = mark_tx_spendable(tx.id.data, conn);

                if (no_row_updated != 1)
                {
                    cerr << "no_row_updated != 1 due to  "
                            "xmr_accounts->mark_tx_spendable(tx.id)\n";
                    return false;
                }

                tx.spendable = true;
            }
            else
            {
                // tx was marked as non-spendable, i.e., younger than 10 blocks
                // so we still are going to use this txs, but we need to double
                // check if its still valid, i.e., it's block did not get orphaned.
                // we do this by checking if txs still exists in the blockchain
                // and if its blockchain_tx_id is same as what we have in our mysql.

                uint64_t blockchain_tx_id {0};

                current_bc_status->tx_exist(tx.hash, blockchain_tx_id);

                if (blockchain_tx_id != tx.blockchain_tx_id)
                {
                    // tx does not exist in blockchain, or its blockchain_id
                    // changed
                    // for example, it was orhpaned, and then readded.

                    uint64_t no_row_updated = delete_tx(tx.id.data, conn);

                    if (no_row_updated != 1)
                    {
                        cerr << "no_row_updated != 1 due to  "
                                "xmr_accounts->delete_tx(tx.id)\n";
                        return false;
                    }

                    // because txs does not exist in blockchain anymore,
                    // we assume its back to mempool, and it will be rescanned
                    // by tx search thread once added again to some block.

                    // so we remove it from txs vector
                    it = txs.erase(it);
                    continue;
                }

                // set unlock_time field so that frontend displies it
                // as a locked tx, if unlock_time is zero.
                // coinbtase txs have this set already. regular tx
                // have unlock_time set to zero by default, but they cant
                // be spent anyway.

                if (tx.unlock_time == 0)
                    tx.unlock_time = tx.height
                            + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

            } // else

        } // if (bool {tx.spendable} == false)

        ++it;

    } // for (auto it = txs.begin(); it != txs.end(); )

    return true;
}



bool
MySqlAccounts::select_inputs_for_out(const uint64_t& output_id,
                                     vector<XmrInput>& ins, shared_ptr<mysqlpp::Connection> conn)
{
    return mysql_in->select_for_out(output_id, ins, conn);
}

bool
MySqlAccounts::output_exists(const string& output_public_key_str,
                             XmrOutput& out, shared_ptr<mysqlpp::Connection> conn)
{
    return mysql_out->exist(output_public_key_str, out, conn);
}

bool
MySqlAccounts::tx_exists(const uint64_t& account_id,
                         const string& tx_hash_str, XmrTransaction& tx, shared_ptr<mysqlpp::Connection> conn)
{
    return mysql_tx->exist(account_id, tx_hash_str, tx, conn);
}

uint64_t
MySqlAccounts::mark_tx_spendable(const uint64_t& tx_id_no, shared_ptr<mysqlpp::Connection> conn)
{
    return mysql_tx->mark_spendable(tx_id_no, true, conn);
}

uint64_t
MySqlAccounts::mark_tx_nonspendable(const uint64_t& tx_id_no, shared_ptr<mysqlpp::Connection> conn)
{
    return mysql_tx->mark_spendable(tx_id_no, false, conn);
}

uint64_t
MySqlAccounts::delete_tx(const uint64_t& tx_id_no, shared_ptr<mysqlpp::Connection> conn)
{
    return mysql_tx->delete_tx(tx_id_no, conn);
}

bool
MySqlAccounts::select_payment_by_id(const string& payment_id,
                                    vector<XmrPayment>& payments, shared_ptr<mysqlpp::Connection> conn)
{
    return mysql_payment->select_by_payment_id(payment_id, payments, conn);
}

bool
MySqlAccounts::get_total_recieved(const uint64_t& account_id,
                                  uint64_t& amount, shared_ptr<mysqlpp::Connection> conn)
{
    return mysql_tx->get_total_recieved(account_id, amount, conn);
}

void
MySqlAccounts::set_bc_status_provider(
        shared_ptr<CurrentBlockchainStatus> bc_status_provider)
{
    current_bc_status = bc_status_provider;
}

void
MySqlAccounts::_init()
{
    mysql_tx        = make_shared<MysqlTransactions>();
    mysql_out       = make_shared<MysqlOutpus>();
    mysql_in        = make_shared<MysqlInputs>();
    mysql_payment   = make_shared<MysqlPayments>();
}

}