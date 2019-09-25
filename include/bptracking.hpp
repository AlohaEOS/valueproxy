#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <string>
#include <vector>
using namespace eosio;
using namespace std;
using std::string;

CONTRACT bptracking : public eosio::contract
{
  using contract::contract;

public:
  void transfer(name payer, name reciever, asset value, string memo);

  ACTION regmodacc(name account, asset max_outgo, std::string weblink);
  ACTION whitelistacc(name username);

  vector<string> split(const string &str, const string &delim)
  {
    vector<string> tokens;
    size_t prev = 0, pos = 0;

    do
    {
      pos = str.find(delim, prev);
      if (pos == string::npos)
        pos = str.length();
      string token = str.substr(prev, pos - prev);
      tokens.push_back(token);
      prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
  }

  TABLE eos_deposits
  {
    name account_name;
    asset total_eos;
    asset max_outgo;
    std::string weblink;

    uint64_t primary_key() const { return account_name.value; }
  };

  TABLE whitelisted
  {
    name username;
    uint64_t primary_key() const { return username.value; }
  };

  typedef eosio::multi_index<"eosdeposit"_n, eos_deposits> deposits_index;
  typedef eosio::multi_index<"whitelisted"_n, whitelisted> whitelist_index;
};
