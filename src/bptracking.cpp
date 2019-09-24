
#include "bptracking.hpp"

void bptracking::regmodacc(name account, asset max_outgo, std::string weblink)
{
    require_auth(_self);
    deposits_index deposit(_self, _self.value);
    auto itr = deposit.find(account.value);
    if (itr == deposit.end())
    {
        deposit.emplace(_self, [&](auto &e) {
            e.account_name = account;
            e.total_eos = asset(0, symbol(symbol_code("EOS"), 4));
            e.max_outgo = max_outgo;
            e.weblink = weblink;
        });
    }
    else
    {
        deposit.modify(itr, _self, [&](auto &e) {
            e.max_outgo = max_outgo;
            e.weblink = weblink;
        });
    }
}

void bptracking::transfer(name payer, name reciever, asset value, std::string memo)
{
    if (reciever == _self)
    {
        vector<string> memo_split = split(memo, ":");
        name account = eosio::name(memo_split[1].c_str());
        print("acc name ==>", account);
        if (account)
        {
            print("account fetched");
        }
        else
        {
            print("no account fetched");
            account = payer;
        }

        deposits_index deposit(_self, _self.value);
        auto itr = deposit.find(account.value);
        if (itr != deposit.end())
        {
            deposit.modify(itr, _self, [&](auto &e) {
                e.total_eos += value;
            });
        }
    }
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
    if (action == "transfer"_n.value && code != receiver && code == name("eosio.token").value)
        eosio::execute_action(eosio::name(receiver), eosio::name(code), &bptracking::transfer);
    if (code == receiver)
        switch (action)
        {
            EOSIO_DISPATCH_HELPER(bptracking, (regmodacc))
        }
}