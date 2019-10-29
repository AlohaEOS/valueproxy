const {JsonRpc} = require('eosjs');
const fetch = require('node-fetch');
const rp = require('request-promise');
const request = require('request');

class ApiChecker {
    #rpc;
    #producers;
    proxyBps;

    constructor(defaultApiUrl) {
        this.#rpc = new JsonRpc(defaultApiUrl, {fetch});
        this.#producers = new Map();
    }

    async loadProducers() {
        const response = await this.#rpc.get_producers(true, null, 200);
        for (const prod of response['rows']) {
            this.#producers.set(prod['owner'], prod);
        }
    }

    async loadProxyData() {
        const response = await this.#rpc.get_table_rows({
            json: true,
            code: 'voteforvalue',
            scope: 'voteforvalue',
            table: 'registered',
            limit: 100
        });
        this.proxyBps = response['rows'];
    }

    async fetchBPJson(producer) {
        if (this.#producers.has(producer)) {
            let url = this.#producers.get(producer).url;
            if (!url.endsWith('/')) url += '/';
            url += 'bp.json';
            console.log('Reading bp.json file for ' + producer + ' at ' + url);
            const opts = {
                uri: url,
                json: true,
                rejectUnauthorized: false
            };
            try {
                this.#producers.get(producer)['bpJsonData'] = await rp(opts);
            } catch (e) {
                console.log(`Failed to load bp.json from ${producer} with ${e.error}`);
            }
        }
    }

    async verifyApiResponse(url, producerName) {
        // const _rpc = new JsonRpc(url, {fetch});
        const get_info = `${url}/v1/chain/get_info`;
        request({
            uri: get_info,
            method: 'GET',
            time: true,
            json: true
        }, (err, resp) => {
            if (err) {
                console.log(`get_info failed on ${url}`);
                console.log(err);
            } else {
                const info = resp.body;
                console.log(`${producerName} | TTFB: ${resp.timingPhases.firstByte.toFixed(0)} | Head block: ${info.head_block_num} | Version: ${info.server_version} | ${resp.request.host}`);
                // console.log(" >> " + Object.keys(resp.timingPhases).map(k => `${k.toUpperCase()}: ${resp.timingPhases[k].toFixed(2)}`).join(" | "));
            }
        });
    }

    async checkAPIs() {
        this.proxyBps.forEach((prod) => {
            if (this.#producers.has(prod['account_name'])) {
                const bpJsonData = this.#producers.get(prod['account_name'])['bpJsonData'];
                if (bpJsonData) {
                    const nodes = bpJsonData['nodes'];
                    if (nodes) {
                        if (nodes.length > 0) {
                            const endpoints = [];
                            nodes.forEach((node) => {
                                let endpoint;
                                if (node['ssl_endpoint']) {
                                    endpoint = node['ssl_endpoint'];
                                } else if (node['api_endpoint']) {
                                    endpoint = node['api_endpoint'];
                                }
                                if (endpoint) {
                                    endpoints.push(endpoint);
                                }
                            });
                            if (endpoints.length > 0) {
                                endpoints.forEach(endpoint_url => {
                                    this.verifyApiResponse(endpoint_url, prod['account_name']).catch(console.log);
                                });
                            } else {
                                console.log('No API listed for ' + prod['account_name']);
                            }
                        }
                    }
                }
            }
        });
    }

    async populateBPJson() {
        for (const prod of this.proxyBps) {
            await this.fetchBPJson(prod['account_name']);
        }
    }
}

module.exports = {ApiChecker};
