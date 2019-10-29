const {ApiChecker} = require("./api-checker");
const apiChecker = new ApiChecker('https://api.eosrio.io');

(async () => {
    console.log('\nLoading producers from eosio producers table...');
    await apiChecker.loadProducers();
    console.log('\nLoading registered producers on voteforvalue proxy...');
    await apiChecker.loadProxyData();
    console.log('\nCollecting data from bp.json urls...');
    await apiChecker.populateBPJson();
    console.log('\nBP Info loaded! Starting API checks...\n');
    await apiChecker.checkAPIs();
})();
