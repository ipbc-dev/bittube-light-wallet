//
// Created by mwo on 15/06/18.
//

#include "../src/MicroCore.h"
#include "../src/CurrentBlockchainStatus.h"
#include "../src/ThreadRAII.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "helpers.h"

namespace
{


using json = nlohmann::json;
using namespace std;
using namespace cryptonote;
using namespace epee::string_tools;
using namespace std::chrono_literals;

using ::testing::AllOf;
using ::testing::Ge;
using ::testing::Le;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Return;
using ::testing::Throw;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using ::testing::_;
using ::testing::internal::FilePath;


class MockMicroCore : public xmreg::MicroCore
{
public:    
    MOCK_METHOD2(init, bool(const string& _blockchain_path,
                            network_type nt));

    MOCK_CONST_METHOD0(get_current_blockchain_height, uint64_t());

    MOCK_CONST_METHOD2(get_block_from_height,
                       bool(uint64_t height, block& blk));

    MOCK_CONST_METHOD2(get_blocks_range,
                       std::vector<block>(const uint64_t& h1,
                                          const uint64_t& h2));

    MOCK_CONST_METHOD3(get_transactions,
                       bool(const std::vector<crypto::hash>& txs_ids,
                            std::vector<transaction>& txs,
                            std::vector<crypto::hash>& missed_txs));

    MOCK_CONST_METHOD1(have_tx, bool(crypto::hash const& tx_hash));

    MOCK_CONST_METHOD2(tx_exists,
                       bool(crypto::hash const& tx_hash,
                            uint64_t& tx_id));

    MOCK_CONST_METHOD2(get_output_tx_and_index,
                       tx_out_index(uint64_t const& amount,
                                    uint64_t const& index));

    MOCK_CONST_METHOD2(get_tx,
                       bool(crypto::hash const& tx_hash,
                            transaction& tx));

    MOCK_METHOD3(get_output_key,
                    void(const uint64_t& amount,
                         const vector<uint64_t>& absolute_offsets,
                         vector<cryptonote::output_data_t>& outputs));

    MOCK_CONST_METHOD1(get_tx_amount_output_indices,
                    std::vector<uint64_t>(uint64_t const& tx_id));

    MOCK_CONST_METHOD2(get_random_outs_for_amounts,
                        bool(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request const& req,
                             COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res));

    MOCK_CONST_METHOD2(get_outs,
                        bool(const COMMAND_RPC_GET_OUTPUTS_BIN::request& req,
                             COMMAND_RPC_GET_OUTPUTS_BIN::response& res));

    MOCK_CONST_METHOD1(get_dynamic_per_kb_fee_estimate,
                       uint64_t(uint64_t const& grace_blocks));

    MOCK_CONST_METHOD2(get_mempool_txs,
                       bool(vector<tx_info>& tx_infos,
                            vector<spent_key_image_info>& key_image_infos));

};

class MockRPCCalls : public xmreg::RPCCalls
{
public:
    MockRPCCalls(string _deamon_url)
        : xmreg::RPCCalls(_deamon_url)
    {}

    MOCK_METHOD3(commit_tx, bool(const string& tx_blob,
                                 string& error_msg,
                                 bool do_not_relay));
};

class BCSTATUS_TEST : public ::testing::TestWithParam<network_type>
{
public:

    static void
    SetUpTestCase()
    {
        string config_path {"../config/config.json"};
        config_json = xmreg::BlockchainSetup::read_config(config_path);
    }

protected:

    virtual void
    SetUp()
    {
        net_type = GetParam();

        bc_setup = xmreg::BlockchainSetup {
                net_type, do_not_relay, config_json};

        mcore = std::make_unique<MockMicroCore>();
        mcore_ptr = mcore.get();

        rpc = std::make_unique<MockRPCCalls>("dummy deamon url");
        rpc_ptr = rpc.get();

        bcs = std::make_unique<xmreg::CurrentBlockchainStatus>(
                    bc_setup, std::move(mcore), std::move(rpc));
    }

     network_type net_type {network_type::STAGENET};
     bool do_not_relay {false};
     xmreg::BlockchainSetup bc_setup;
     std::unique_ptr<MockMicroCore> mcore;
     std::unique_ptr<MockRPCCalls> rpc;
     std::unique_ptr<xmreg::CurrentBlockchainStatus> bcs;

     MockMicroCore* mcore_ptr;
     MockRPCCalls* rpc_ptr;

     static json config_json;
};


json BCSTATUS_TEST::config_json;

TEST_P(BCSTATUS_TEST, DefaultConstruction)
{
    xmreg::CurrentBlockchainStatus bcs {bc_setup, nullptr, nullptr};
    EXPECT_TRUE(true);
}



TEST_P(BCSTATUS_TEST, InitMoneroBlockchain)
{
    EXPECT_CALL(*mcore_ptr, init(_, _))
            .WillOnce(Return(true));

    EXPECT_TRUE(bcs->init_monero_blockchain());
}

TEST_P(BCSTATUS_TEST, GetBlock)
{
   EXPECT_CALL(*mcore_ptr, get_block_from_height(_, _))
           .WillOnce(Return(true));

    uint64_t height = 1000;
    block blk;

    EXPECT_TRUE(bcs->get_block(height, blk));
}


ACTION(ThrowBlockDNE)
{
    throw BLOCK_DNE("Mock Throw: Block does not exist!");
}

TEST_P(BCSTATUS_TEST, GetBlockRange)
{

   vector<block> blocks_to_return {block(), block(), block()};

   EXPECT_CALL(*mcore_ptr, get_blocks_range(_, _))
           .WillOnce(Return(blocks_to_return));

    uint64_t h1 = 1000;
    uint64_t h2 = h1+2;

    vector<block> blocks = bcs->get_blocks_range(h1, h2);

    EXPECT_EQ(blocks, blocks_to_return);

    EXPECT_CALL(*mcore_ptr, get_blocks_range(_, _))
            .WillOnce(ThrowBlockDNE());

    blocks = bcs->get_blocks_range(h1, h2);

    EXPECT_TRUE(blocks.empty());
}

TEST_P(BCSTATUS_TEST, GetBlockTxs)
{
    EXPECT_CALL(*mcore_ptr, get_transactions(_, _, _))
            .WillOnce(Return(true));

    const block dummy_blk;
    vector<transaction> blk_txs;
    vector<crypto::hash> missed_txs;

    EXPECT_TRUE(bcs->get_block_txs(dummy_blk, blk_txs, missed_txs));

    EXPECT_CALL(*mcore_ptr, get_transactions(_, _, _))
            .WillOnce(Return(false));

    EXPECT_FALSE(bcs->get_block_txs(dummy_blk, blk_txs, missed_txs));
}

TEST_P(BCSTATUS_TEST, GetTxs)
{
    EXPECT_CALL(*mcore_ptr, get_transactions(_, _, _))
            .WillOnce(Return(true));

    vector<crypto::hash> txs_to_get;
    vector<transaction> blk_txs;
    vector<crypto::hash> missed_txs;

    EXPECT_TRUE(bcs->get_txs(txs_to_get, blk_txs, missed_txs));

    EXPECT_CALL(*mcore_ptr, get_transactions(_, _, _))
            .WillOnce(Return(false));

    EXPECT_FALSE(bcs->get_txs(txs_to_get, blk_txs, missed_txs));
}

TEST_P(BCSTATUS_TEST, TxExist)
{
    EXPECT_CALL(*mcore_ptr, have_tx(_))
            .WillOnce(Return(true));

    EXPECT_TRUE(bcs->tx_exist(crypto::hash()));

    uint64_t mock_tx_index_to_return {4444};

    // return true and set tx_index (ret by ref) to mock_tx_index_to_return
    EXPECT_CALL(*mcore_ptr, tx_exists(_, _))
            .WillOnce(DoAll(SetArgReferee<1>(mock_tx_index_to_return),
                            Return(true)));

    uint64_t tx_index {0};

    EXPECT_TRUE(bcs->tx_exist(crypto::hash(), tx_index));
    EXPECT_EQ(tx_index, mock_tx_index_to_return);

    // just some dummy hash
    string tx_hash_str
        {"fc4b8d5956b30dc4a353b171b4d974697dfc32730778f138a8e7f16c11907691"};

    tx_index = 0;

    EXPECT_CALL(*mcore_ptr, tx_exists(_, _))
            .WillOnce(DoAll(SetArgReferee<1>(mock_tx_index_to_return),
                            Return(true)));

    EXPECT_TRUE(bcs->tx_exist(tx_hash_str, tx_index));
    EXPECT_EQ(tx_index, mock_tx_index_to_return);

    tx_hash_str = "wrong_hash";

    EXPECT_FALSE(bcs->tx_exist(tx_hash_str, tx_index));
}


TEST_P(BCSTATUS_TEST, GetTxWithOutput)
{
    // some dummy tx hash
    RAND_TX_HASH();

    const tx_out_index tx_idx_to_return = make_pair(tx_hash, 6);

    EXPECT_CALL(*mcore_ptr, get_output_tx_and_index(_, _))
            .WillOnce(Return(tx_idx_to_return));

    EXPECT_CALL(*mcore_ptr, get_tx(_, _))
            .WillOnce(Return(true));

    const uint64_t mock_output_idx {4};
    const uint64_t mock_amount {11110};

    transaction tx_returned;
    uint64_t out_idx_returned;

    EXPECT_TRUE(bcs->get_tx_with_output(mock_output_idx, mock_amount,
                                        tx_returned, out_idx_returned));
}

ACTION(ThrowOutputDNE)
{
    throw OUTPUT_DNE("Mock Throw: Output does not exist!");
}

TEST_P(BCSTATUS_TEST, GetTxWithOutputFailure)
{
    // some dummy tx hash
    RAND_TX_HASH();

    const tx_out_index tx_idx_to_return = make_pair(tx_hash, 6);

    EXPECT_CALL(*mcore_ptr, get_output_tx_and_index(_, _))
            .WillOnce(Return(tx_idx_to_return));

    EXPECT_CALL(*mcore_ptr, get_tx(_, _))
            .WillOnce(Return(false));

    const uint64_t mock_output_idx {4};
    const uint64_t mock_amount {11110};

    transaction tx_returned;
    uint64_t out_idx_returned;

    EXPECT_FALSE(bcs->get_tx_with_output(mock_output_idx, mock_amount,
                                        tx_returned, out_idx_returned));

    // or

    EXPECT_CALL(*mcore_ptr, get_output_tx_and_index(_, _))
            .WillOnce(ThrowOutputDNE());

    EXPECT_FALSE(bcs->get_tx_with_output(mock_output_idx, mock_amount,
                                        tx_returned, out_idx_returned));
}

TEST_P(BCSTATUS_TEST, GetCurrentHeight)
{
    uint64_t mock_current_height {1619148};

    EXPECT_CALL(*mcore_ptr, get_current_blockchain_height())
            .WillOnce(Return(mock_current_height));

    bcs->update_current_blockchain_height();

    EXPECT_EQ(bcs->get_current_blockchain_height(),
              mock_current_height - 1);
}

TEST_P(BCSTATUS_TEST, IsTxSpendtimeUnlockedScenario1)
{
    // there are two main scenerious here.
    // Scenerio 1: tx_unlock_time is block height
    // Scenerio 2: tx_unlock_time is timestamp.

    const uint64_t mock_current_height {100};

    EXPECT_CALL(*mcore_ptr, get_current_blockchain_height())
            .WillOnce(Return(mock_current_height));

    bcs->update_current_blockchain_height();

    // SCENARIO 1: tx_unlock_time is block height

    // expected unlock time is in future, thus a tx is still locked

    uint64_t tx_unlock_time {mock_current_height
                + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE};

    uint64_t not_used_block_height {0}; // not used in the first
                                        // part of the test case

    EXPECT_FALSE(bcs->is_tx_unlocked(
                     tx_unlock_time, not_used_block_height));

    // expected unlock time is in the future
    // (1 blocks from now), thus a tx is locked

    tx_unlock_time = mock_current_height + 1;

    EXPECT_FALSE(bcs->is_tx_unlocked(
                     tx_unlock_time, not_used_block_height));

    // expected unlock time is in the past
    // (10 blocks behind), thus a tx is unlocked

    tx_unlock_time = mock_current_height
            - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

    EXPECT_TRUE(bcs->is_tx_unlocked(tx_unlock_time,
                                              not_used_block_height));

    // expected unlock time is same as as current height
    // thus a tx is unlocked

    tx_unlock_time = mock_current_height;

    EXPECT_TRUE(bcs->is_tx_unlocked(tx_unlock_time,
                                              not_used_block_height));
}


class MockTxUnlockChecker : public xmreg::TxUnlockChecker
{
public:

    // mock system call to get current timestamp
    MOCK_CONST_METHOD0(get_current_time, uint64_t());
    //MOCK_CONST_METHOD1(get_leeway, uint64_t(uint64_t tx_block_height));
};

TEST_P(BCSTATUS_TEST, IsTxSpendtimeUnlockedScenario2)
{
    // there are two main scenerious here.
    // Scenerio 1: tx_unlock_time is block height
    // Scenerio 2: tx_unlock_time is timestamp.

    const uint64_t mock_current_height {100};

    EXPECT_CALL(*mcore_ptr, get_current_blockchain_height())
            .WillOnce(Return(mock_current_height));

    bcs->update_current_blockchain_height();

    // SCENARIO 2: tx_unlock_time is timestamp.

    MockTxUnlockChecker mock_tx_unlock_checker;

    const uint64_t current_timestamp {1000000000};

    EXPECT_CALL(mock_tx_unlock_checker, get_current_time())
            .WillRepeatedly(Return(1000000000));

    uint64_t block_height = mock_current_height;

    // tx unlock time is now
    uint64_t tx_unlock_time {current_timestamp}; // mock timestamp

    EXPECT_TRUE(bcs->is_tx_unlocked(tx_unlock_time, block_height,
                                    mock_tx_unlock_checker));

    // unlock time is 1 second more than needed
    tx_unlock_time = current_timestamp
            + mock_tx_unlock_checker.get_leeway(
                block_height, bcs->get_bc_setup().net_type) + 1;

    EXPECT_FALSE(bcs->is_tx_unlocked(tx_unlock_time, block_height,
                                     mock_tx_unlock_checker));

}


TEST_P(BCSTATUS_TEST, GetOutputKeys)
{
    // we are going to expect two outputs
    vector<output_data_t> outputs_to_return;

    outputs_to_return.push_back(
                output_data_t {
                crypto::rand<crypto::public_key>(),
                1000, 2222,
                crypto::rand<rct::key>()});

    outputs_to_return.push_back(
                output_data_t {
                crypto::rand<crypto::public_key>(),
                3333, 5555,
                crypto::rand<rct::key>()});

    EXPECT_CALL(*mcore_ptr, get_output_key(_, _, _))
            .WillOnce(SetArgReferee<2>(outputs_to_return));

    const uint64_t mock_amount {1111};
    const vector<uint64_t> mock_absolute_offsets;
    vector<cryptonote::output_data_t> outputs;

    EXPECT_TRUE(bcs->get_output_keys(mock_amount,
                                     mock_absolute_offsets,
                                     outputs));

    EXPECT_EQ(outputs.back().pubkey, outputs_to_return.back().pubkey);

    EXPECT_CALL(*mcore_ptr, get_output_key(_, _, _))
            .WillOnce(ThrowOutputDNE());

    EXPECT_FALSE(bcs->get_output_keys(mock_amount,
                                      mock_absolute_offsets,
                                      outputs));
}

TEST_P(BCSTATUS_TEST, GetAccountIntegratedAddressAsStr)
{
    // bcs->get_account_integrated_address_as_str only forwards
    // call to cryptonote function. so we just check if
    // forwarding is correct, not wether the cryptonote
    // function works correctly.

    crypto::hash8 payment_id8 = crypto::rand<crypto::hash8>();
    string payment_id8_str = pod_to_hex(payment_id8);

    string expected_int_addr
            = cryptonote::get_account_integrated_address_as_str(
                bcs->get_bc_setup().net_type,
                bcs->get_bc_setup().import_payment_address.address,
                payment_id8);

    string resulting_int_addr
            = bcs->get_account_integrated_address_as_str(payment_id8);

    EXPECT_EQ(expected_int_addr, resulting_int_addr);

    resulting_int_addr
                = bcs->get_account_integrated_address_as_str(
                payment_id8_str);

    EXPECT_EQ(expected_int_addr, resulting_int_addr);


    resulting_int_addr
                = bcs->get_account_integrated_address_as_str(
                "wrong_payment_id8");

    EXPECT_TRUE(resulting_int_addr.empty());
}


ACTION(ThrowTxDNE)
{
    throw TX_DNE("Mock Throw: Tx does not exist!");
}

TEST_P(BCSTATUS_TEST, GetAmountSpecificIndices)
{
    vector<uint64_t> out_indices_to_return {1,2,3};

    EXPECT_CALL(*mcore_ptr, tx_exists(_, _))
            .WillOnce(Return(true));

    EXPECT_CALL(*mcore_ptr, get_tx_amount_output_indices(_))
            .WillOnce(Return(out_indices_to_return));

    vector<uint64_t> out_indices;

    RAND_TX_HASH();

    EXPECT_TRUE(bcs->get_amount_specific_indices(tx_hash, out_indices));

    EXPECT_EQ(out_indices, out_indices_to_return);

    EXPECT_CALL(*mcore_ptr, tx_exists(_, _))
            .WillOnce(Return(false));

    EXPECT_FALSE(bcs->get_amount_specific_indices(tx_hash, out_indices));

    EXPECT_CALL(*mcore_ptr, tx_exists(_, _))
            .WillOnce(ThrowTxDNE());

    EXPECT_FALSE(bcs->get_amount_specific_indices(tx_hash, out_indices));
}

TEST_P(BCSTATUS_TEST, GetRandomOutputs)
{
    using out_for_amount = COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS
                                    ::outs_for_amount;

    std::vector<out_for_amount> outputs_to_return;

    outputs_to_return.push_back(out_for_amount {22, {}});
    outputs_to_return.push_back(out_for_amount {66, {}});

    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res;

    res.outs = outputs_to_return;

    EXPECT_CALL(*mcore_ptr, get_random_outs_for_amounts(_, _))
            .WillOnce(DoAll(SetArgReferee<1>(res), Return(true)));

    const vector<uint64_t> mock_amounts {444, 556, 77}; // any
    const uint64_t mock_outs_count {3}; // any

    std::vector<out_for_amount> found_outputs;

    EXPECT_TRUE(bcs->get_random_outputs(
                    mock_amounts, mock_outs_count,
                    found_outputs));

    EXPECT_EQ(found_outputs.size(), outputs_to_return.size());
    EXPECT_EQ(found_outputs.back().amount,
              outputs_to_return.back().amount);

    EXPECT_CALL(*mcore_ptr, get_random_outs_for_amounts(_, _))
            .WillOnce(Return(false));

    EXPECT_FALSE(bcs->get_random_outputs(
                    mock_amounts, mock_outs_count,
                    found_outputs));
}

TEST_P(BCSTATUS_TEST, GetOutput)
{
    using outkey = COMMAND_RPC_GET_OUTPUTS_BIN::outkey;

    outkey output_key_to_return {
        crypto::rand<crypto::public_key>(),
        crypto::rand<rct::key>(),
        true, 444,
        crypto::rand<crypto::hash>()};

    COMMAND_RPC_GET_OUTPUTS_BIN::response res;

    res.outs.push_back(output_key_to_return);

    EXPECT_CALL(*mcore_ptr, get_outs(_, _))
            .WillOnce(DoAll(SetArgReferee<1>(res), Return(true)));

    const uint64_t mock_amount {0};
    const uint64_t mock_global_output_index {0};
    outkey output_info;

    EXPECT_TRUE(bcs->get_output(mock_amount,
                                mock_global_output_index,
                                output_info));

    EXPECT_EQ(output_info.key, output_key_to_return.key);

    EXPECT_CALL(*mcore_ptr, get_outs(_, _))
            .WillOnce(Return(false));

    EXPECT_FALSE(bcs->get_output(mock_amount,
                                mock_global_output_index,
                                output_info));
}

TEST_P(BCSTATUS_TEST, GetDynamicPerKbFeeEstimate)
{
    EXPECT_CALL(*mcore_ptr, get_dynamic_per_kb_fee_estimate(_))
            .WillOnce(Return(3333));

    EXPECT_EQ(bcs->get_dynamic_per_kb_fee_estimate(), 3333);
}

TEST_P(BCSTATUS_TEST, CommitTx)
{
    EXPECT_CALL(*rpc_ptr, commit_tx(_, _, _))
            .WillOnce(Return(true));

    string tx_blob {"mock blob"};
    string error_msg;

    EXPECT_TRUE(bcs->commit_tx(tx_blob, error_msg, true));

    EXPECT_CALL(*rpc_ptr, commit_tx(_, _, _))
            .WillOnce(Return(false));

    EXPECT_FALSE(bcs->commit_tx(tx_blob, error_msg, true));
}


TEST_P(BCSTATUS_TEST, ReadMempool)
{
    // stagenet tx: 4b40cfb2fdce2cd57a834a380901d55d70aba29dad13ac6c4dc82a895f439ecf
    const string tx_4b40_hex {"0200010200089832f68b01b2a601a21ec01da83ffe0139a71678019703665175d1e067d20c738a8634aeaad6a7bc493a4b5a641b4962fa020002fb1d79f31791c9553c6dc1c066cde2361385021a5c33e7101d69a557cc3c34a1000292b1073ac7c2fdd851955e7fb76cbc7de4e812db745b5e795445594ea1c7d2a721010a9e82db48149442ac50cff52a39fda95f90b9683098b02e413a52b4d1f05f1c0180f3a3f93aaac0d8887ce542c01e49afbaf1f5ac8e1e4c8882011dd83fa33d1c9efa5f210d39e17c153a6a5825da329dcb31cd1acae98961d55038fb22e78fb93b95e6820a6904ff5b7bbfcf0cd1f6b9e9dd56f41062c7f5f43d9e1b3d69724eeca4bc3404a41ff0669f7f8ae725da8bbc2f8d0411605d2c3e959e3099414ffce1654418020d6ee74bb25042045e955b966e060a9f65b9cfccfbdcb4ee3101019a51f3a41372414c90383c9b84fb4ac690047ea67d70c917192abeb1fdf6c618aeafc732ae9a3e6c6f7fb274240ca04d24035a1d55b3fd1060dc3f4e64f9816868f36d2a0aeb76b64169bc9525d7d70b077c8b998b1204c6108e2917af893b0de24f4adb043eee433b096db8ef730de6e339a84f966b80b239821f350e2a8b10817fc19508da4f82bcb90fcf84cd47929ab1ba90bde5d252767736a4b23d26063cc6b2120dc636b2d4e3e7b3f6d7448882e01c10cf7bdd2117710dc49a59b49cf99212800093da22a6ab5bf693a80d4a662f35259f088a91a26ac261980c537238daa7a40477871f7481973edbcc86fa4885a4061d5c889d0986b4c0cb394165d4039081069f61cff822c7a227474d1609dbf05909f738803834b9e1b5ee68dcb825fcd906d880684e5fa684c8cb02495530fcaeeb35a405345f352ef3c8cf276c096d4303ee14cc0a48533afd1bcdf32dcfb25db4c45a19b10926dff72ace2c2a1d0759090acff99ad06e3f81c4d759a9f74d47fff9cb61d3784aa0eb57a250eec8d14800b85026f5b112740e2e8c257541bdfa4ea1f166b9d930e9705fa1d3f3c2be1e0fb242394d9230c7c457fc8422a6f919a06df5c52d954fa1bfdb57a0a2778d35020ef07490fc3c4f6e99ceaef97fcb3da049898944f3291b96a4f9ce491654340414565db2e0c6bd13eab933194b32336c09c478689cc4a7160b63ecb59937b2028e1649744c6ea69db01aed72fb8b39519cb28d764f158789cc539e01fd5f8f098597c8977e6d2b22c37b289c95dca13bece9e28f741ae2384ead8f0d6df7c30f311210523cb0b7e8c1173aee311362282f3c7d06ae2153969d073bec14ff3605d193bfe829e110df39b894fc4a8eb462d357098688748d579c85f9dc9e9cd20229c9865bc54ae8338433247c28875249e37d4d9b9ccbad611ce5948f1c7ea00fccdc41a9fbe942539a44111f160c9b1dc2653ecae0e4c30bada58d9164fa4c021b60fc0e2068510ef8ec56f7c8e036f2eb31ae0871c9a6aff7b8ad6a86054a06daf06393871f41859f51165f09d22fedd8a2326099a803491dbff5027f72540ed6981f505d1162d86c1ec716758bfbf4e1bfdbbe628b9340942a328209c86b0ec71981c90b407b146cbf065cfd8b0a1c2528aaf007c505a83779dacf8cb7500577a1f8f4a2c9a833d08b254c0c5edbbe87c9eaf266627d23bf518373dceba4099c13c6c468b99c16d56ea5eec12620867fdeb01be66bb0248a7ab7dd6434aa042392c4482e8694292bfc638f1a344d35c9cbfe121a25d2302db4db9688285905fcef9a892593840fe2ddebf0e059e341b2e7f107ee6a19fe386e3f9ea0da2801c5e9fe1b9c4b21155f6f2194ef36f2383097f9cf7ee0994873cfd92d098a33006547ef67bbeb2c1386fcbeec6f0b974532f2ac6e474b26683a37ed3919ec3108b578d3a552c32a362c0bea8bfe35c481f014721af9503a0663d95f5aa91b8802c32f60dbaa65978ad114a2fd45901a07fb4dded61d19e30c8c11e4efbdc1180a02d08d05a646bd8a4d160203da8d490ae0a0b4e631280401d635eb78e0416408c208436dcd653c11a75f365e2d012e0e69f0d4fe2fb2997726dad29468109001ed7c65464ba734e4b35f2ac85a4437fcafe0cdb6128e634970ba420e2e3296080636520e9fbf453e07cdd215117a2eeffed5a0ea0003761f5eb4c35aebf87b063f59d0571e149dc9d0c143a59415a7fe950d34b5024087145969abdc7f7140079bc25119825dda1bf080150bd40142d9635fd87e5dc691899d2686ccf6c9d106c7cdbf783fbbc83fc0feebe8abee3b458e7417a385215f1f89a2a89bef3b870c7a30a6cf4920ddd6eb50439c95c1f38fec867a41e628415ed142b7014991fd0818e8cf725b7075498e773790ccc882db0eb22453f0c9931debfb8d3adb09370f30e62b5fe94ffa7e55ff053488ad48f6e244ed6f043fae8f2225a024800c690ce15dd8017457d23c834b276d079d4a141f22db8fed4ea8f1e378fb741a2da30c09abc817ef0ad589428f966b8bdf92fb32fefa67f425ef1c58a2d4c881e8cc0156519256b87736ed84691b585687971c01a23da37ab76726602a01e771d9820add53f62168d51c7ec12ede030b61155f12e7c8d5573985a9e0884bcc41e0b50aa066b3337f2ab8e3b1031a1c843700413ef03d3734536547fc636055f5d15b0f804b3d25f90e4aaf199ee666d90199d0781526bb2040144b5a638411c1067d0bc5e3ef1911c5223fb0f5d0ce6387702b6957f9466b1a8be6879b8c7190ec1e0662cda6005a363f5d3d4585e6d9d15349be797353179b0a713b925dddfbd05609430601339bf31226cce63c763d887364aa6f1d640892f2a074338b741b7c3303e62c6fc16f544ceeed251dbaed838cbc978eafbf1f837543876f611d64d354049936220af27edd4834df2740842f033821147619098b45b5819b49cccc772506d01ed8f4ee1877c3368e3863f4e60422ea8918758ebd8989b0e26041a906db0ade4365e7d322dad4f40b489b53fed41ed2d421c1571c7311d0357a511eff490193afe446f183baa147e12dcac9827391ef233396dce9465ab6d1e073a128cd062fa829dc3011e72d05f96956976120c973da24b874aa7dab15e6ae58272efc0ba97355b22d5ec283957b2dec2926bfc2f66059c0146ca91b53b0ffc03f3bf402c99ada7e18d713bf57351efe369ea204354343d3b080e8a386931ed541b7e3059f2bf1736b4903a25f0bba187f5553768586202d0cec388dabaaeae9b7ade100733139f77682ffab54409e14605ec50f5648da42702caa1d27bafc7c40c3b30b9ab2a54810e1e4584701b58b3bed1a4d820bb13308e9d5485c011cbb1cafd0021c0e888147666a3c00a59000f9195d6d9e423e11d7c4eb69d71541c7ad943601aaf1f7c6b5c0e058cbc989baa7f3a1927031e2219b398e2483a6b77a5d6e7e029c2ba36bdb882927cc06adc89b312bad1c5e72f9a27b48fc3a2c8ce431882c034a345c2692a323dc87380d62a1afdb73fa47d9b5ac0e495138e0dc7ee57835079ee2e9fae9b4e1148ef7df42e1fe213b99f573ca222430771088e05266db80092bf53d23c800b8f15b036883cfe58fac0682aca8d58d5ef2cb0c0bbf3c39560a347d51d3d1444b98b175af1abe341ece49c69b7022e61274a3d75f81871d780aef74495569cf97c7f4144f43a87b90cefdb6ca3e4ce8b12b168120499847000e67389d13f8cc190762f0c02ab0230e45ab1d53d59f665fdbe73c342169bede097c85f596595cce0d8d40a02ff89f81b64d7126c6bb73c3ff9739dc5af0444f01ab1818cc5f2c6c548873fde9095ca9efc2bb3f4c818fcb873ef23007a1253704ad561b872f774b6cf8dae848f98cb0af3d4b3c45acebccae094edc02b5250a0e259f8470ee88c12fc16014b5403fd3dbe29895aa411af2f1292ff20aa2fcf9038ee4453a51448cc7ff4a076d8c635619711ec70e8c43926f7d0df8f89ea156069bb64117f5be57b731dd3d573721e966216e98b71464342d95e712dfed27f50ecb291728f205428d335f3f4cd3e4657c2dfd66cceee48576f64d921e521f6d0d67df90d553e4c6751cdca6e425f1994fc9f1676a261f831ce5b0238c1a2ec10c6c94c5d22951c47b01c7e43b2e2199e0af412bfc025dc5d2d6ed6d154f154f0f9c7351c25d184ea619fdca59820448ee49a46e63ef871ebcb23af8abc58e9708bdb984519ae76303e661f0c00b44c22b6ec9bf95830fb047bc38106aad060607c6e1c4998b0cf07ba5b293cc96621a3151a67c7272e38f2e62f69528d1d19b060eb8c6f165d95dc13aa2e28b2844dc63bb3e24ac06eed20050251b698dd3f103e2f309501092c1b537656af79a0bbb2f398a2b0d8d7c976308db90dde5cc2b0a7ee510e902624a7c19f1169b6eb84773e131b6dade39030bb22555fa8ed9c904422269debded88e1c51b4294a458a080b78d4c76d72a49cfa988e96e3c3f900a86dcaf5953907318e9cf30b95fac95724df8982387a13f4f92cfa21226a7be0cf3d414afc4a7ff632bf803ff6061fdda12571de6c85477562da2e7cfdf70f9029210fd211239c5242002798598c278145b9501d3c41c12f998a7c82a6a8cdf06adf87910e52f43c79993edb24bf691d52a6ea175150cba134eb3fce090c3e401900e2671b4e5995408f22d9a7fa615301ac6153b69718b7e0079e3b31d3690075967bc173f6e716c2c30ee24863b96df0b1f7ec9c1fd7e7ec7d019bf4273ad0459d3c182ef8603f6b83d702f545cef5b1604f16b02657c94a3cc18323bca2708a8105bca731db97385576e84d78cd3f02e18144f2861bfe43590f5b203679007b199f0033dc73acc50950d2ef9f7678131f562599bdb4e91b97e673e30e7290eee0d5758dc3c1bd11c07df261222e095d182201524c271ee877a7afcefb40400197c1a153e4741edf9eb5602e856247f6845aadad9a3f3c1a3947099fc14890b02129d3382955faad6d4acc3186cb9749300d8ea9b32ec95ac0a8f11515e430b49b12829fa4e4e2e2754fd43313c544cb5b3521c3bdfc9fdfcfa9c35a4e79103a5c4f1b3b5e150f0f95951a4f9f7e43d6f2734b7ce212a1f14d70b2131c30009351dc1caeeac4c546240df2180dfb49dd0c780222d76caa4769fbe03fc826c055a409f05286a61f760ff281e095574472db33b803543f471f79bc711feb4a9052a4236308c714a2555b909047dee9421e3a9d04d4e7785077f71541a46084d0c72c83528bd367b260bf4baec560cab216f6a5ab82164bff19e6b0532005db300feb20a02e1eea6d9d31b4e2b0415295b9fe9c16650f25588ea94f0472328ac03bfd43876076e94c08506673db0b2fadcc91e1dfeb2439c9863e3063bdb833507bad8d987160ec288207c20a0df18b4c9b1b00b1d0feda3505c72e3c63f72c20ec41c2e4e377dce092671ea2f2ba7cb7b291d1ae230f8b26ad3b9ac0fbcf2c602f808c5729f1234dec272e59929673e32b3b4effeeb6fa74b33f211f70b80210670f0866ff75fc199e1ebfdaafee3468a818e1eb7fed5dae34520bf3e1b0c2105afce4636a482be6c9b51a4d27a170d22c6da8ae6324d6ea81f2bed486aab0f01b3eca20a60e739ebfab2cb6a374fa7287768b9635242ce1ae5dbef3879922c06817f3ecf8a08bb64f00c04a820a718748c714661d2f90b90113926a242c7bf092e54b99799098f8d4a10b52117f9b1cc044133a47eaf8b235d89db626407280766fe232f52ba22bc7c19113d43ffc4bfeff57b0dd811db81760403a11221c2056e9ded4f7e88f277158dc94ed1135d9196e61cc80e259381a7f3bc1135eca40fb91f963d6c38bf83d7b9b8dd275a146491a7fb0cb69539c3659f5360ede7170b49c57ea2d4ec258a8792cd2e7746c8955b79e9ac1e5ce0c0498952c3b91616011e98ebae173c7b74ca004c0fb2000b990a7a776f89b12204d02b3362778a67026e448e3c546bae75889435cda08570c72a52d7a30524185c21b5774c9727220408d5f57c713fd33aa2747ec5d61e7852a94db2b8b5d2ebbf90fd3a12b490830e7df75affa5d202f352b4e73128776b08a2fb415d1a354a3dc69c4845faedc007bb1856af1d47839959f012c44c0294505177938c6b4ba98939efcc69aa87fa029b68a2308e53d79dc013b557e19ebfda4bc93b4101c3596cf137c68cc7746702b53a6e2a2d557aacf8e7ef7df513bd85da45c1811529e4ab5e0bdc601f73b000338ca3699d1935b1a4d5a7e9fd36bcbbd9963d3a264c09d11fa76d1bdaa3480bc6230f56760cd25a47e0a384f1f3e921575e24f2cf9ab10d144822fab4e5210c83ca06824acbc8c4acdfe709cfa250185d935b38c4dc5d7e9b9bcd37a61cef3768a2961ed44c9ba5706b9b95e4fd24627fe34ca79483a2a04a483b5a69c297618dd52d9f107ceded42a02818d55f5a322df5d8c1a85591ef6a001357ecd8a25289e619626154939d5c02a3e1b82987411e648257958259d93b220ddc24aeaaabb6286f67ba87cb18bf546cd5008def5780f9af7d7b817fa2d30f9b2f4c667f1e63f9494e32945b4ca1fee45ef4d8fb7448550f5ded771ae79ec0ba48fb0a2b7ab4d7500e24ce807834e708c7afb9197cebba8755db2f06869b317374158c8f2d31c3d35ec57b0408331f5aa18da7bd620360b83b1ffbc47019ab09a0fa18d12511e22449a0ccfd24cccbd2edebb149f5b59bffe31bcd6ac5132d2c7bb804414165d17770f4eb8b8253d1cd393c4dc3d9216a538f31a4db9864157d0bdb900581dd3d85f45f36124c8f4ecf876d09cc626108b0769cab79357e6fe34ec1aeb94007a5416e8dcb13b0143db8da0fae95284b872c562f9f272e8d29eaaf09c8a4a3b1bd72892bb3ad3a4c4462969ccf431a40f3213940fa530c1adbb2d595e8ef94aee36de8ed09087d769cb8ad966404d7f689b60e020e1a977fbe70e57a31c096814d3f1b5e924d55ef79259599dadf4ff376d5d120c2d5aad9e14a92b8b656fe915a294aca796cbe6b15d4b0dec70aff9988a00514582afbb519f64ccc0f989c5fddfa9b2f50b37653e3aed60213339abfc6d0d58808d0b677e98248e0db2e64ac23b371ca2f9d2662a7c359d57ad770dc7ac4fd6f5652fd4e8dc66f8254969a37fbbfb4b2197ec864c18e1cf515f6b2646cf8fe5ed55427807a7aac9016cbe811c86f0b973916d5123dfb8e8aded328a464139ee92fd22c69d60fce6906b2174938a88b62fe96a1c8f977330c548ac479622b2c89db22099d5b1d1931c1bd15c49ad655d1371bcbed1c42734b3ffdcccf40b00b9be3e7cf6f5b8da625a450d1cd489360140022a468fed9be5bf8eeca7ad220ebca7e4de8ccc9d5ca2a8112d07a0014f8491bffa6968f6a70789c22a49ccaca63f196c063a29e69f03399cb7ff7f50bc572bb0902bf43d9c741202779c6e41b3e32c3082ef3a212450f0eabbf61e943d7d1a7dfd7a53c229e1c6e22a6d1893367b2b803ed23825888f799d73cc6471d16157f70c226fa39dcca4a08bb6fb3a512ff06b797f869007b39691f2cea01360fc5eb0925715f36cc246135279ee360522090e88ac7993cbd9581333c2a872a5755dd241af1e0fe04ee5857d16494222a32e4d9a1d07890492ce8320f164f0980ea133024f370e914fdb5b1ed02f174d0fda91c4a0f5cecb369745e5d7e4bf0fd4f0a2f55f5cdd5e5d20dd5076391c78e6f97e7c930f6bdacf40bd5a2edd56d3b68ed4027bba853f0342192a379999127dcd27c73fa21e06d37a6b6b5bc39e8d7c2189621f38e74a1fdad551586919d4193a3ea024160460ba1ecc2c9104e43a23bd24baeeb3adc43543d904d98c0560a246f7f21adc0fd21205dd4f2bbb3f4a9d16c4ecda3f558548caaece8dcb08e97d8f086e51d559505128753ccb7d2dffe54e3f7d5b63bfc9472105529f5c1ad027da671eaaf3d6045fe0b556e236a17d15b7d6a916a6ba91bf2e75045fcab0dda68803371c08b576563aa779a3c3f7b739c5c3e2cf6ece351e790633bbfd7055e50e93f6436aa7c4dccc986c6f0603ad20e9d810e1296b89a3270dac70d7d6cabe6bce51085895ddf4c2f6a8c63ce53c14fed78b3f89e30452a9478bd1195a9fb990c4f27c2d18f7bb54bd459549d6908255eac8437ca24a51a7b050714cd034961e851fb90efcb89a81e3212a0bef071ab6102906a9c0735f0d56f496011c39c25bb03e146f98121020f58ebc06da4537dd79e48b016561392b8047fd4047d9c5c0afe1babd8f32c6292fc36b1e79c0ae14c42ac2fee2cb108d40c6d91f264a3046485a3adaee2e23b9a34bd6f5b8c73667b7983ed1dfecbf927203d9afce3066bc7fa9ca4ae8d234192d637740f088d43b0296017bba1530c726c1c337c00ab288b97376a6a97ffb05e6596b8955c22c3501e373cbdbc54dfb1bf60367840c895d2cdca3513a1701c10e1d11e79d80bc098cfd9ea04d4296f4da2bd398f52ab76bea76730d82e76836aed863644fbe5c8a4f22aa7552de4d53e71d42e51b4f59851025cf3e4c1a8b57c2e026194f104b0865354683b09a13765451ce31198cc5e20ca54c1b7142bfedf03a7763d3eb0cc4a0ff71049a4556ab962caa0b3b8dd8bc78eb227e27280f789b728b416c2df52fb672628d7822e7757415003d1975634dff7ea0ec3bdb2bee4bca1de669fe68c3349a590cc9640c976b8d3580acb183ad2a9b9923e3ce7f2679d1656d921f4615c48466b28a8fcf309215eb1d6546084a7c53d837a59fe0e5fad510bc7691c29880284b696f387d5cd2214c08ed00f931a412f747354f98682f6c19e4c42e07d8a75729b8578a46a9d3a998d94b21a0c9fb235d18d9c172162341ea2c0533b267b692b670f6eca96e4087d4dbed18bd2cc7a7466527266210919dcaa8ae1e19549ce14a5e511e3d1f59836b4e09e59f016cdbae1b715030b9662bb7060f05ad0bbee62373d074a9b53f483732777c612cfd4c275fff73d8a501bdf7a0c2eb8494deb2195a59b6b0f99c50a8e99069d56ec104aca17f0ebcc880f76c6f285ce3d03c2f0be55d836f45f188c841c691d720f2d2365c0e23709e370efbc8eeb942bbdcbef23aeb118631187239b9bbcd70de0175e98b684ee6023b96702ebbf1ae3f5ca8a9dca6498afd0252c44930b1c90f9d3ba43f4aa6a75ea0191ed61e6e210ef71a2a7389c8c8f947ff668d85e2564bb07510f9a8ba1c58674a1910d0c4eafbe00574a8308b19df2f8b474ed5f81b080a1c9a07edb216f236b0219c5e4d5f24aedcb8f1807c55bc508f2cbc9a3e30458a0ca4e4ddf138897220d757c876c50f1fd14173e6358d338c922a4438809d606bc9d91816a634331ee025f65760f6650fc63c9c8a5326f0e1f0fb1916425dc108e684ff10d621b7ce10fc6633c7432db26bd713b532a7d9788db88c42f5c84c8f576b83b49b2bad9fb02c5a0740aac474154a6205270d67406e4b5d80af40896d696a0faebca9c910609d32b03f1b23ac722b1fc1801e5d0fe4778abd4f2662d025fe7905e9b38b3e0057d37c0bc65f9a6982fb2d3db32382860c2371ff463c9c81fb991703b548c9e0f777ac72ceec2149405126f9dcc5f18cba19012ace4f8e936e4324f30eb366d07726f143546efaaaa8309e027325b19e4f2b34481fe8edba09cfd67e48e201c0c251bf9218d0d2db8286692e1c558c59abf1b563ef17828830423164e642873080a20476073dfe969afbf47b04f69b44364de220a7212dde70ed18e5d50688e070190894c8bd3b3f06cd38280ec280849316bba88b6925bda2ba3531a8ff6100f6ec2124282045555a2131281abf07d2b36f2aaa4c60c98fbc5a72e716f66b00353c282443c1ce8bd23974be486e30269eb8e4583e62f1b9ed15d1bd180f5190e5b72ec9729b8d2c32285776e8a84aa32150bc08a12e0b09c3055181052f65c0d09844ed2bf6ec9158f648196acc5c5e740f6c7093436943ba181e60b0b525b0615db23ac58b1b67acbf1a0f59ea1a9f7ec9769e7ea0519aeb3ba1e8d783a2a05fe8a34bcd88749249e0d7ba8a0bcd1cdcaa7d836d2534585eecf38a1dcd5360d292e8a9c7c1c25c90a8fc4eefa3f22392809fcf6764c06ddb169b0058b505a08f7c38f4f8e8c12c888729c23dd9ae33df30917120150bd59720ce5dc169eed0edee1b0f9ecba4ad43ff6ee0e1103c784bcf1e4068a348111f0f4ed7397c8720fdb777c4206f8b70da082a2f8d2e92220229fa6c55402119861e671e8c9f31807b52d3380a8bcb1d29bc9795989fdaceb185839dc16cf8b115b3e0ba96eb4ae0b29762bd84ac30f8afe03f0ac7140515f28aaeedb50e98e51ee16236aa8410c0065c3813512229c2d0a96de8ed252074715386e5a5eba1b93d77cf344c9466e05ccfd6892160efd60a40af3f94ac81625acac363ccea77665eb6bfe33a69c570af107fd693722c0e9439c36ddd7d3a4a2edf496cafead3455cd23b69190fcd201c1db6315c8c03592c86a4ad3c17515eb30ebcdc42ed85099ad99aca3af87d601137cc4c899dd7b860bbcecb1e804d4ae784dedd01b3ed1e04d74e86160244c08619121bea73d7afd858287683958bde5319f5d1020700f52ffbe564a6e8e9109c1f8423ba825c221be5e9be8cb1d6b74827770c365c024b8989b18a45c612b0263d0a7c3cc19bc6c048c5a60fb228de2f43196a8802dc44eede70cff7d84cc0af37dc7829830e7d888136543b070d9a99472c1f70255bf70ddc9c0299fbeb007d92096ab2018d68144def74c223cf1d845a2c5b850613307ffe27eb2497ce80cde0d7c8243f3ac86f0459e3dbdfdae11a0168209e86bb90d0b47a5b91efa28093fe2ca554772cace53c08fac18d003be5d79afa78e7888057dd473761335bf007b0dc9cb3d79122c67fe5dfb6b3715a9412ec7b5b4b9dd95733d86a63442530f6a21f0b32249b40584cf3b3aacce380d5d3eae24f42fec00e6e9f73813526a07152cce67f8e46d89b3f6c74fb564d43a1ca94e9f04a3d406a635656ea020c4088f6102da9534922ba27d53281adf30b7336616060884c31c7f83491fe3ba5104341d2897549feb54172bf1ca72e6d95263e025c4698b1530076aca0089e59103e55ed49b46c356ca5458f2bc6789c2ca6b8062545dd830c5f71768f82ebaab03be78c77035c67a567689ddc3cc8726ee7811af7ee8c06e157d4c87a909c5840fa19359e07395b820876d40d80a7b4aff426b8e4e2803ab1fadd1b630e6c1db0de6809988ca5a56233d318f94204a126a605942d0628e7cba88ec227d4018db057b45a3ffff61be5e26c713a4362e7515a4e0b3d7f4132b6f5543e76c22a28d0eb4359c2aa0ac26651ac69385350cd5f308c84db05cc0ce991d31d04d18c6890d79826561017d38bb2ae636aca2eec453436d4510a4d52d571865ee4aea36c0070d167ce0035fb2aff4a7c450b60e0fc92d6e7405bb7b299d14537027b260340f1576bde7c27fdfcdd4bcb51c96e2272ebccc93fc834f3328668f27130dfae60976e6e17ce6cb4a4a2a108e58c539e90172c0109141883e0bb9dacc2b7df1810f84454f4e185538749a986ee94b34c2587d36813f06496b5c78a85a9fa9bc300e22d8f8055afae6504cf54ee99dfcefad61c4f314f1a9d831cffd498e066a9d0c389276cf18bf590091e78cefc8c657ac60e22ee8febc0acbccfbb6d0bc756d018fd57533905c01d33f684be06b6e8ac5d3164c2473f2d2bbc2cc6f33ae3d980ced83c2f9691090dde73ae3d94ccf5a02a32e5866f9457321b2c0c6b2932d100d288012fca3b8caf82080a7b254395bac134277699f0e91ccbe429df8504ae00d334e471b530642a542d470280b1157e9edd978be004bdbe1855148969f1b7607656fbeaa3dbf98da4103fee661c049f1a8cf0f722d2ca188a18614048514b90eb3988115b87b3b120363defec35214322fa1478530525cad835fbc075145ec089aa2a8efc02bfc11466e5355d955dcbf88fb74aabfdbaad82ca18ca8bc80a703169aaba1f8b1de6d036e5be656f8739f04f0172f91d0a3c3cff6d0e9f265dd0a0bdd5f9e954ee4534646adcbf4d157a90ec734dc4a0c13eb728135b29daaba02d5b2ac38b390502f181e756c69590f6c6bf2e6430cfa2e6d2f9675a219501b0c6a5006d1ea075a196e039fc9b4460a22ef2645bd98ddb11300c4d76ef0646c04725320885933e206a70c7f069408f5ef3c3bccc195e2f901cdfe81b7f01a870332aff32cf339f1bb2601ba45d062bb0fb1c1f182a55260b19c18aa65205fb409af3b27d5c85be5a508bf735a02853ad7269c63636cc3ccd0ce4935b500aa720737def7452e39b3235d0e115961de40c174063883becbbe895ac13fc6574df7045418ea8a8dbcf512172efcd5dbfb6688d949da143dcf082d879945901dfbb601406e263f616bed1e969964e000f74e9c8ca04fc8e4f064071daa34121d6b5c0b8886a328eb8b922bd1c4c511ad867f498ad96841ac7fa2280b6ae0be1c439d0dda808cd74cd5aa8698588d622f830a898beeadbda5d0ce5c71d39c479e9a0e0f5a481b848bff2c6668d80d6ef9668531d814d8caf51ab6c18eeb0fd688f0800e4416f24f4094e76cd19e9f083ee00ecf4275ee01e6f9073d4a621e1d98ee040ec72a8bfc1483478ea24dd45ea01ee8a5c516ea50970efb0e3f21bbd5737e6e09ae78488130cdb3e4fb52f54e3f644b45f2c88d35a28a1ec9b009afd3bb8c410bcc09f411680d7181084b8e4a58099512fab865f39673bd81901adccafbcdb409b43808c79551548b10d201cf8922fe5da8c2154d601007980aa7407489fac50cb5a734cde8580749968d53efcf066856d190543ff5e4bd8d02b66346cce24b0d9f33afafa9c899ae7145c1f7b5b0e85090dea52379a0121e6566903078982300fe9d03c7b57335ebfbaf31f52e720ace0c467703c27844084635060815cb3c0b986ed0b406cc389957548f04d70eb2a8e4daf0a49e9a54f707acb0a3c0f81d0780595f9a2e58f618693ce928ff54f2a4a47601c08532959c6aef0915b484fd0ed1eada9fcc2616e181d3de0b5e95c880d866bfff792e5d3e5549b80f705e94095927a1fae3a2038724b5de59a786dc8bc3d3286c5e7c114b9846345c004a6d05491f29f8fe23d6daec6ea5f89de526b0549686f8ea05a950e045fe1af95281097cdf8052039ae20553f0bb12a875b507b3c394515709acf465113c51a361cc0c9d8f621010abdb43e2b96f1b5fe05b507b70e80b9382e3129b0bea448dcec60d78eba7fe0db4bd25d96cb8a8a5faba3d0ee8963fdfbc0cdd7ad16f97c5709103f62e1b3b015ead01cc315291c3c2304b592e104fe8a008d54dc8fc15a7ac39087d63f055e7b87b9312d58cf3bf4858a2cf9a9319339888b680783cb902c9fb0a111ec094ca9f8ad33863bee0d0c243769b5fa8d57e81cf7077e256785fd3c703b4d1dcd47a004d8a0dbdc1ba9a76bb8631b29bd329b2742465b07862fe833308f67aee1667d3a7be0d67d912b997dc6118b9f4dd759dadf1a325f7cc534aa405a1641d10fc313e6fb0e57a5c2e2e603c5dee3896319fd1142ab0894a8904090f64817ace87388361e7a5b0e418a366308e7504d0347daa7daf90213b39c94b0bcfa9dc63d5b87daa690edf897b5c4aaccb614f267071853b7f6f92992fb22f0e42fa32681c44e181783cae74b54682689153d1bf358a8c3eca0eca65ff48780e7e760fdb82b1bdd4169c37b685d35c2f64b9f2a6b009f62324ba6da8bd4a7606d1940c5700a1a57229d82e6989414ae937f60dbf0a275ba10dda13b2c4502a01a5690fc2ea2ac5cdd445570e568a64f7a7871f9694eb3905a1bf0dae9ded1408024ad299d2f2d04bd6cab1e7beea5abc6e3cc7c87226d93f329007fec0099004da76d8ea1c8eedea6664f762a410e97c527ac8189ee310fd3136f5a6d552100b3f0d1572d84249d88a218e5e8b73df39a8a9fe14d900827a857f3aebef534c0c8bd3a42443826ed5c4ca4ba21591f080f6fdd304103cadaccc461638cf23b90cd8ccd1916603a6e9bdc086372e91bf5125e15bd837c0f9a6a691e6da704cf408dc8c88ee168eaba1d057648bb59d04d69c67df87cbd0142a6d5bd87fbf4d2d0f6fb0adb1aa1652dcca6a03788a4296abbb0d688daabcdd1963e7b98954f6be0ab1413978513504b9e5b2038438a1460cc3199068b916a7349f78881395e2fa08317db264572687c8891a3e6f41617d6448bba0bc17c116221d3542dc73f91b0ab490218da61eeae4b62b5f389bbc2d8252c2114db65dbed68a0405e0d153af0a344b8b68fa2f054402fba9c2ec5b6ce536be91b1929fd4ba3d8e20a2d4d7c1007ff87f7df5b4c057ee42c371188b63d9a1e9d1e63cbd296bdbff51c204cbe800c342558408a4434d554af4030855ddd0ef0295417c669615a7635e0d4a3f08024077d4bcfed78faa15c805cadfad0d0bb1f628dbc424a1e7a945e8908438d2015e716c83cdf57f8d6065fa86182c80577213f07437d9bde259a0e54414af410b6516de6eeda634d241486db5966a4f4aeb9d18e92dc9fd9f4fbd9a4a7ae15d0ddfcb8f4f6a5bb544f045217c36345fe43b64aa0c5d08ba1104fd13b57d070806e7d8a83731fee819f5e8f5a892ccdb64fde073476966d0d8ff13123b3d24040db42fbee63630b0a86858cfa4231844bc6176f36e6fcbd94fa014c321b5a05d0dead8501cf0015adc4e933bac66696e6bc92ef1345ffbb823207df4869b5d720e6fbcab98627a54501421da7a00ceb02973fe2daca9f78d41ac28c0bde44b2b0905733b8424df52ffb0e67b2d67e0fccec31f6a5a682dbbe42886c621d2da8c0cee5ec5c48a6f5400ceb2607b9d1094ad665100a84c35c5bb82eeb9ffa0695e0afb40b5e1605d58c8a50dbfb91a961b387f43b263eaacb365bec80b42615e33009ef7dd83787b0740e09ab385c991720b003377d97da4c3fe273bf7d35389980a19182e174a5b1cd8ffc83be319f819b6a5269c96bfc467bf3414d9260bc2e10b9a8fe95b539397ca8fedabe611c63db9130e8183a77bd346ff4520bc502cd323b5d10463dffab6b3278e73e5771ec7d2b103a66650333f58f04119036fbd34c42a021cfd12f2d68b19d1aa373e4bf6621dc2b7c42d0b27ce6fc01199ad5f34e74ec954dc8bd7342e573a58a8880ce28ca83801519e9dd7a3063db98f60bac11045815fd39fdaf1c1890bcb355cee46761fc6bc239c3584f1482054a9e345626f6a1b142fef1970da21a2be3f8e052e4e0f9a0f176099606d6e29dafb5a735fddd81cf462e40c424daadf72ec02f70e2aec1d18b7de4ef9d1ef9e364dea63e4a4080d720b6631268537be3b74c225782608fd2f0bf71c70fb84d88f0bdf2c0649a77e94cfddf710fee9d39e765746e09f3437c665c6c962a9043a798cb564f83e4fb5c99ad93ae89f0856c74a36c0fb7512770c91b83e40cf4960b128cc009e3aba2abf5685ea44453f4a2708f793503f4905423d4cce0a135f923ffc3176868ea7fd2cbb1c7801ecd483024123f61ed919f58f6faab196283417cc575741d348a98a31e9188768115a0eed01b8f47fa9b62a89e821c581cbebeb0069f2c7fa75ec25a8dbbbdabfa130b3443afeb03416671c872bf99ea7f87655304c5270e047a719716554ddd0142a529e6c49b67d41e78bed286e3137698313ab321321866f97f4ea898d23c3a37a941b19702530118fa752a06b7a1e4c61f1f3d9633b73b0c068ecd20e040ce4e0d2632c27dd41faf65144aeb30658a9171dbe7daabd36afddb03e4526e20d201ffd849c42cf3146b0860a0514367a579df4c0fd9dafea9949810711ed7be822d32fc901ae389693fe39e5b28750e510231457e09512d5f97733e99576a421cbf99a51ae3cbd7126423dff294b3b037f3a0aeeaba2ca6a8e1c58fc5b72f1f70415d0fe46c2871e73ab7a4f50a9dfba30c415c1e45af4dd48a7af8b0b0ba49409527e1b2835fe61e43610681e170d73681519efd0ef1775588ed5991b33f69fd378919193ceefe6e99b0fbc55fb2a0db84b054e1e1add6061a3fe18131257ad27e5af6338a7bf79e3497d966ac9906b935e539a4d9d94adc4302561c6725219ca4e6642227c6f331424750ae48189ca4d54f8cef0a328d6e51a0e4073bd745fa07bf77899578c200eac6e66d19245db9d6f8b8437fef9a7afbb69d1a99de13a5dce22c3f4fe2bff36f73e5243b1cee4bc155afa6301f3704c90e5d075fa079d178ee92691ce2152da8765b5f38f82b186d7bce1c853a81c7b8407e0906e88df7b754d58035c5d0536f7d6ade45fb47fd489a5a07ef58e9956e10308a399d2511fdb8f1548c341f7c1eb2ec632a54ea30019d256b595213a869b4502bf1d2806910ef2a31c2eb09fbca65598e1503d911f2ff0cf6c4d906cd421b71ef909d7ad6322a38ecf7d061c212dc1fd5106bbdb726e9007f78c94f2d628096502c5849f8b1c3e79a316fbb6951a39858b333e9d393b3c28d0de43b798c68cd8a245b1e0172c997dd5b8ae89da1154a60ead79f6b2c4151305cd417b05ed13703629de6212edc92322ca841febd3d6d0557d4e26fda4e0f6184ff06bc026f2c7f2b1023a4e30692682ad39ef70201c66bbe1d8bc39cc2f8a9363fee904e6aacc2eca41034791af9be167cf62d1a1c94b9ce32c9415527b43f5837505d93aff050fb8fe6ed47b96eaa81561e4ca29d6a02a04c9671d603576aa492b40c014369c9a9933ee07e3836502a3e7bf5ed99fb35f65e2e0ddc3dd441258cd4d9b161f1dfc53ae5699a3788e169eb585fd91276945dd41363044db68f86aa133349db78f34d54ecb163afe3379edc7dce02238e27d799cb0ba5e1b300235b6e8bc7e450484b07eef5c24954e4df7b75b04e73740e2403b3d37c73138765cbcdc5307c051dcd4466a7e21dbb0c6f3a30e07e89d057e6697c3fe848309d98a050bffb811d158301eb381c3ddc87af73831334db6f788f6d24afbcf83abf9c3a9b1714842f44329d74ff49d6ff5264a1b283ed91a9606919526a7515e105c7481e1468624fca6c6c1db4d389d87e196363421fef88338dcbf683b4540a484fcff7abcd6b5e5864454283c511be98f5fbd78180adf8a168f20aabce0667e3e672cd4de67b1d942538ef557218f1c4b9f45f53b20cf4ae1f001a56c230e363c8df87bb6d78c312be74f5da0b15bb18a6f219b9d52ccc3b1edf85958c492900204e9f13574a88550e2a771e588aff9e61a87d39c5929f6492a41bd289311c7d8740c4a91b6fa0c06432384476f068c0035a57c19c8730e5768bcfbfd9a5d6392f2bb4693990b19faab771bd6cad9e0f7cbec70ba3a9cbec9adcb821e709aa0ad74d921961179e33272ee98eef61d4755607517cc9db96e3bdaecf0c2aba155ebf03abb603c7913d6da5860bccc17ec6f072d2a95f51f41c2bd77e8d8bd997d68b624bc47037e259de8a72188cf1f86f2f36b56c92769998055c5dcfb0a352bbd4753369d3cf1a85ab820557f5f8e3fe76de3e5e6ed3dafcf66b4cd8a82375823d6ac2fdf3f0a10c8c03010d3309452b6a9b8bf4e3a7f47c01748b574d1257f0aa18e6c4732e2e69566101c0ea0b6437d2d64c28902c18351306ce6f1e62e7319e614113b2f426102ec80ca2978d8990943bae86d2077f35019cb83cc4156e12376733876447e0e91ac58000b93344e7409f3eea4afbe6309e23d18c0f1db8b41b6206edcb8c3cea650ce609a62d5647eb8320cc8a40d4ef9b2f900aad835174707f4ec0503ee701d280733d38bbc5e8ea2fa6716bbfa946a9add1b54e3471accb65c9c835cd6912b44dd343a077b0c7e14b89007b15d3192f811bff695929dfd824c1881718287d6e8b3e4a31573dd5b8b9d683e35144ea22ae99eb46fae356d97f6996d01b3353c06bb9b2323fe18f4c2b84d39a3732b38e9bc494cc43e07e64ddfc101fefc1aee9fcbcdc03b1d9120e7fc59633a6ca9f041b32cd43f4f37deaa30ca04542db96cf77cf08805361d04cf373d253ad4727d59968ddb4fa1b33474a6da08c39801a33725631aeef8196795b8c6aa6d56b60706666bc57b22849fd3fa500c27ddced7ea0334f9b92d590696a8270acfa03af69bff0f0b8268d374fa219d0cd224811c9454c83c533bc2719a08aa96be554a0f95fa4ca7c02894032245400b211a9eae98d96052a04fd9d7a8bb382b48468e6d5b0fc861e14ed137ceae560bc0cb0a7dc0af76a97b245966b22597162f4ec6c9f3221a4a69ef43fe19011e0aebcd2d760a352b3cfda10caf77ec902a8ae7bec3700899c9d726dba2df21f103bb703d885b7ca78ac746ca3e99831b4e2f7dd999ec44cc6922bea346618e6001f51f387c2cfaecdc740db9e44ecb4b0da370208ecf3eba1c15ea70bb06d63404025b000aada40e40e1e737966d7729c5084dc8e9c6392dcaec97b4dd6e1b8b004831345902ca2628784ad7ee435e803d31b5275931210b5ea4fcd5132fb8eb07d79fdaa8ce14a5ebbe21d7c2388841207a6920f04c6184fca780803a97930a0f5107450f0e50b530753f8a7efe8c9f298cc00b5b585e49217aa82da904ef33095421011f97c377aaf76f2390c6ba3405754e311e35810701f20c15ad15bd930c08f3224945681dd53e49094d286f89a0cf29bd90feac7a616ba6cb52f47f910a"};

    std::string tx_blob = xmreg::hex_to_tx_blob(tx_4b40_hex);

    vector<tx_info> mempool_txs_to_return;

    mempool_txs_to_return.push_back(tx_info{});

    mempool_txs_to_return.back().tx_blob = tx_blob;

    EXPECT_CALL(*mcore_ptr, get_mempool_txs(_, _))
            .WillOnce(DoAll(
                          SetArgReferee<0>(mempool_txs_to_return),
                          Return(true)));

    EXPECT_TRUE(bcs->read_mempool());

    xmreg::CurrentBlockchainStatus::mempool_txs_t mempool_txs;

    mempool_txs = bcs->get_mempool_txs();

    TX_FROM_HEX(tx_4b40_hex);

    EXPECT_EQ(get_transaction_hash(mempool_txs[0].second),
            tx_hash);
}


TEST_P(BCSTATUS_TEST, ReadMempoolFailure)
{
    vector<tx_info> mempool_txs_to_return;

    mempool_txs_to_return.push_back(tx_info{});
    mempool_txs_to_return.push_back(tx_info{});
    mempool_txs_to_return.push_back(tx_info{});

    EXPECT_CALL(*mcore_ptr, get_mempool_txs(_, _))
            .WillOnce(DoAll(
                          SetArgReferee<0>(mempool_txs_to_return),
                          Return(true)));

    EXPECT_FALSE(bcs->read_mempool());

    EXPECT_CALL(*mcore_ptr, get_mempool_txs(_, _))
            .WillOnce(DoAll(
                          SetArgReferee<0>(mempool_txs_to_return),
                          Return(false)));

    EXPECT_FALSE(bcs->read_mempool());
}

TEST_P(BCSTATUS_TEST, SearchIfPaymentMade)
{
    vector<tx_info> mempool_txs_to_return;

    mempool_txs_to_return.push_back(tx_info{});
    mempool_txs_to_return.push_back(tx_info{});
    mempool_txs_to_return.push_back(tx_info{});
}

INSTANTIATE_TEST_CASE_P(
        DifferentMoneroNetworks, BCSTATUS_TEST,
        ::testing::Values(
                network_type::MAINNET,
                network_type::TESTNET,
                network_type::STAGENET));

}