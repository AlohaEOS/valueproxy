
#include "bptracking.hpp"

void bptracking::registeracc(name account, asset max_outgo, std::string weblink)
{
    eosio::check(has_auth(_self) || has_auth(account), "missing required authority of contract account or registering account");

    whitelist_index whitelist(_self, _self.value);
    auto whitelistitr = whitelist.find(account.value);
    check(whitelistitr != whitelist.end(), "account name needs to be whitelisted first before registering");

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

void bptracking::whitelistacc(name username)
{
    require_auth(_self);
    whitelist_index whitelist(_self, _self.value);
    auto itr = whitelist.find(username.value);
    eosio::check(itr == whitelist.end(), "account name already whitelisted");
    whitelist.emplace(_self, [&](auto &e) {
        e.username = username;
    });
}

void bptracking::reclaim(name username, asset amount)
{
    require_auth(_self);
    deposits_index deposit(_self, _self.value);
    auto itr = deposit.find(username.value);
    eosio::check(itr != deposit.end(), "account name is not registered yet");
    eosio::check(itr->total_eos >= amount, "Reaclaim amount is greater than available total eos for this account");

    action(
        permission_level{_self, "active"_n},
        "eosio.token"_n, "transfer"_n,
        std::make_tuple(_self, itr->account_name, amount, std::string("reclaim transfer")))
        .send();

    deposit.modify(itr, _self, [&](auto &e) {
        e.total_eos -= amount;
    });
}

void bptracking::transfer(name payer, name reciever, asset value, std::string memo)
{
    if (reciever == _self)
    {
        vector<string> memo_split = split(memo, ":");
        name account = eosio::name(memo_split[1].c_str());
        print("acc name ==>", account);
        if (!account)
        {
            account = payer;
        }
        deposits_index deposit(_self, _self.value);
        auto itr = deposit.find(account.value);
        eosio::check(itr != deposit.end(), "account name is not registered yet to send EOS");
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
            EOSIO_DISPATCH_HELPER(bptracking, (registeracc)(whitelistacc)(reclaim))
        }
}