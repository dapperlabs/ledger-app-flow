/*******************************************************************************
*   (c) 2019 Zondax GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#include <fmt/core.h>
#include <coin.h>
#include <crypto.h>
#include <parser_txdef.h>
#include <parser_impl.h>
#include <iomanip>
#include "testcases.h"
#include "zxmacros.h"

bool TestcaseIsValid(const Json::Value &) {
    return true;
}

template<typename S, typename... Args>
void addTo(std::vector<std::string> &answer, const S &format_str, Args &&... args) {
    answer.push_back(fmt::format(format_str, args...));
}

std::string FormatHexString(const std::string &data, uint8_t idx, uint8_t *pageCount) {
    char outBuffer[40];
    pageString(outBuffer, sizeof(outBuffer), data.c_str(), idx, pageCount);
    return std::string(outBuffer);
}

std::vector<std::string> FormatPubKey(const Json::Value &v) {
    std::vector<std::string> answer;
    std::stringstream s;
    s << v.asString();

    uint8_t pageIdx = 0;
    uint8_t pageCount = 1;
    char outBuffer[40];

    while (pageIdx < pageCount) {
        pageString(outBuffer, sizeof(outBuffer), s.str().c_str(), pageIdx, &pageCount);
        answer.emplace_back(outBuffer);
        pageIdx++;
    }

    return answer;
}

std::vector<std::string> GenerateExpectedUIOutput(const testcaseData_t &tcd) {
    auto answer = std::vector<std::string>();

    if (!tcd.valid) {
        answer.emplace_back("Test case is not valid!");
        return answer;
    }

    uint8_t scriptHash[32];
    script_type_e scriptType = script_unknown;
    sha256((const uint8_t *) tcd.script.c_str(), tcd.script.length(), scriptHash);
    _matchScriptType(scriptHash, &scriptType);

    uint16_t item = 0;
    uint8_t dummy;

    switch (scriptType) {
        case script_unknown:
            addTo(answer, "{} | Type :Unknown", item++);
            break;
        case script_token_transfer: {
            addTo(answer, "{} | Type : Token Transfer", item++);
            addTo(answer, "{} | ChainID : {}", item++, tcd.chainID);
            addTo(answer, "{} | Amount : {}", item++, tcd.arguments[0]["value"].asString());
            addTo(answer, "{} | Destination : {}", item++, tcd.arguments[1]["value"].asString());
            break;
        }
        case script_create_account: {
            addTo(answer, "{} | Type : Create Account", item++);
            addTo(answer, "{} | ChainID : {}", item++, tcd.chainID);
            const auto pks = tcd.arguments[0]["value"];

            for (uint16_t i = 0; i < (uint16_t) pks.size(); i++) {
                auto pubkeyChunks = FormatPubKey(pks[i]["value"]);
                for (uint16_t j = 0; j < (uint16_t) pubkeyChunks.size(); j++) {
                    addTo(answer, "{} | Pub key {} [{}/{}] : {}", item, i + 1, j + 1, pubkeyChunks.size(),
                          pubkeyChunks[j]);
                }
                item++;
            }
            break;
        }
        case script_add_new_key: {
            addTo(answer, "{} | Type : Add New Key", item++);
            addTo(answer, "{} | ChainID : {}", item++, tcd.chainID);
            const auto pk = tcd.arguments[0]["value"];

            auto pubkeyChunks = FormatPubKey(pk);
            for (uint16_t j = 0; j < (uint16_t) pubkeyChunks.size(); j++) {
                addTo(answer, "{} | Pub key [{}/{}] : {}", item, j + 1, pubkeyChunks.size(), pubkeyChunks[j]);
            }
            item++;
            break;
        }
        default:
            addTo(answer, "{} | Type : ERROR", item++);
            break;
    }

    addTo(answer, "{} | Ref Block [1/2] : {}", item, FormatHexString(tcd.refBlock, 0, &dummy));
    addTo(answer, "{} | Ref Block [2/2] : {}", item++, FormatHexString(tcd.refBlock, 1, &dummy));
    addTo(answer, "{} | Gas Limit : {}", item++, tcd.gasLimit);
    addTo(answer, "{} | Prop Key Addr : {}", item++, FormatHexString(tcd.proposalKeyAddress, 0, &dummy));
    addTo(answer, "{} | Prop Key Id : {}", item++, tcd.proposalKeyId);
    addTo(answer, "{} | Prop Key Seq Num : {}", item++, tcd.proposalKeySequenceNumber);
    addTo(answer, "{} | Payer : {}", item++, FormatHexString(tcd.payer, 0, &dummy));

    if (tcd.authorizers.size() > 1) {
        addTo(answer, "{} | Authorizer : ERR", item++);      // TODO: is this valid?
        // FIXME: Missing test cases
//        uint16_t count = 0;
//        for (const auto& a: tcd.authorizers) {
//            addTo(answer, "8 | Authorizer {}/{} : {}", count + 1, tcd.authorizers.size(), a);
//            count++;
//        }
    } else {
        for (const auto &a: tcd.authorizers) {
            addTo(answer, "{} | Authorizer 1 : {}", item++, a);
        }
    }

    return answer;
}
