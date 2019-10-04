#include "bptracking.hpp"

void bptracking::registeracc(name account, asset max_outgo, std::string weblink) {

    require_auth(account);
    check(max_outgo.symbol == symbol(symbol_code("EOS"), 4),"symbol must be EOS");
    check(weblink != "","missing weblink");

    whitelist_index whitelist(_self, _self.value);
    auto whitelistitr = whitelist.find(account.value);
    check(whitelistitr != whitelist.end(), "account name needs to be whitelisted first before registering");

    registration_index registrations(_self, _self.value);
    auto itr = registrations.find(account.value);

    if (itr == registrations.end()) {
        registrations.emplace(_self, [&](auto &e) {
            e.account_name = account;
            e.total_eos = asset(0, symbol(symbol_code("EOS"), 4));
            e.max_outgo = max_outgo;
            e.weblink = weblink;
        });
    } else {
        registrations.modify(itr, _self, [&](auto &e) {
            e.max_outgo = max_outgo;
            e.weblink = weblink;
        });
    }
}

void bptracking::removereg(name account) {
    check(has_auth(_self) || has_auth(account), "missing required authority of contract account or registering account");
    registration_index registrations(_self, _self.value);
    auto itr = registrations.find(account.value);
    check(itr != registrations.end(), "account not found");
    check(itr->total_eos.amount == 0, "balance must be zero before removal");
    registrations.erase(itr);
}

void bptracking::whitelistacc(name username) {
    require_auth(_self);
    whitelist_index whitelist(_self, _self.value);
    auto itr = whitelist.find(username.value);
    check(itr == whitelist.end(), "account name already whitelisted");
    whitelist.emplace(_self, [&](auto &e) {
        e.username = username;
    });
}

void bptracking::reclaim(name username, asset amount) {
    require_auth(username);
    registration_index registrations(_self, _self.value);
    auto itr = registrations.find(username.value);
    check(itr != registrations.end(), "account name is not registered yet");
    check(itr->total_eos >= amount, "Reaclaim amount is greater than available total eos for this account");

    action(
        permission_level{_self, "active"_n},
        "eosio.token"_n, "transfer"_n,
        std::make_tuple(_self, itr->account_name, amount, std::string("reclaim transfer")))
        .send();

    registrations.modify(itr, _self, [&](auto &e) {
        e.total_eos -= amount;
    });
}

void bptracking::deduct(name username, asset amount, std::string memo) {
    require_auth(_self);
    registration_index registrations(_self, _self.value);
    auto itr = registrations.find(username.value);
    check(itr != registrations.end(), "Account name is not registered");
    check(itr->total_eos >= amount, "Deduct amount is greater than available total eos for this account");

    registrations.modify(itr, _self, [&](auto &e) {
        e.total_eos -= amount;
    });
}

void bptracking::transfer(name payer, name reciever, asset value, std::string memo) {
    if (reciever == _self) {
        vector<string> memo_split = split(memo, ":");
        name account = name(memo_split[1].c_str());
        print("acc name ==>", account);
        if (!account) {
            account = payer;
        }
        registration_index registrations(_self, _self.value);
        auto itr = registrations.find(account.value);
        check(itr != registrations.end(), "account name is not registered yet to send EOS");
        if (itr != registrations.end()) {
            registrations.modify(itr, _self, [&](auto &e) {
                e.total_eos += value;
            });
        }
    }
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (action == "transfer"_n.value && code != receiver && code == name("eosio.token").value) {
        execute_action(name(receiver), name(code), &bptracking::transfer);
    } else if (code == receiver) {
        switch (action) {
            EOSIO_DISPATCH_HELPER(bptracking, (registeracc)(removereg)(whitelistacc)(reclaim)(deduct))
        }
    }
}

