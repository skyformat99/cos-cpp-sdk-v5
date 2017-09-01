// Copyright (c) 2017, Tencent Inc.
// All rights reserved.
//
// Author: sevenyou <sevenyou@tencent.com>
// Created: 07/21/17
// Description:

#include "op/base_op.h"

#include <iostream>

#include "cos_sys_config.h"
#include "request/base_req.h"
#include "response/base_resp.h"
#include "util/auth_tool.h"
#include "util/http_sender.h"


namespace qcloud_cos{

CosConfig BaseOp::GetCosConfig() const {
    return m_config;
}

uint64_t BaseOp::GetAppId() const {
    return m_config.GetAppId();
}

std::string BaseOp::GetAccessKey() const {
    return m_config.GetAccessKey();
}

std::string BaseOp::GetSecretKey() const {
    return m_config.GetSecretKey();
}

CosResult BaseOp::NormalAction(const std::string& host,
                               const std::string& path,
                               const BaseReq& req,
                               const std::string& req_body,
                               BaseResp* resp) {
    std::map<std::string, std::string> additional_headers;
    std::map<std::string, std::string> additional_params;
    return NormalAction(host, path, req, additional_headers, additional_params, req_body,  resp);
}

CosResult BaseOp::NormalAction(const std::string& host,
                               const std::string& path,
                               const BaseReq& req,
                               const std::map<std::string, std::string>& additional_headers,
                               const std::map<std::string, std::string>& additional_params,
                               const std::string& req_body,
                               BaseResp* resp) {
    CosResult result;
    std::map<std::string, std::string> req_headers = req.GetHeaders();
    std::map<std::string, std::string> req_params = req.GetParams();
    req_headers.insert(additional_headers.begin(), additional_headers.end());
    req_params.insert(additional_params.begin(), additional_params.end());

    // 1. 获取host
    req_headers["Host"] = host;

    // 2. 计算签名
    std::string auth_str = AuthTool::Sign(GetAccessKey(), GetSecretKey(),
                                          req.GetMethod(), req.GetPath(),
                                          req_headers, req_params);
    if (auth_str.empty()) {
        result.SetErrorInfo("Generate auth str fail, check your access_key/secret_key.");
        return result;
    }
    req_headers["Authorization"] = auth_str;

    // 3. 发送请求
    std::map<std::string, std::string> resp_headers;
    std::string resp_body;

    std::string dest_url = GetRealUrl(host, path);
    int http_code = HttpSender::SendRequest(req.GetMethod(), dest_url, req_params, req_headers,
                                    req_body, req.GetConnTimeoutInms(), req.GetRecvTimeoutInms(),
                                    &resp_headers, &resp_body);

    // 4. 解析返回的xml字符串
    result.SetHttpStatus(http_code);
    if (req.GetMethod() == "DELETE" && http_code == 204) {
        result.SetSucc();
        resp->ParseFromXmlString(resp_body);
        resp->ParseFromHeaders(resp_headers);
        resp->SetBody(resp_body);
    }

    if (http_code != 200) {
        // 无法解析的错误, 填充到cos_result的error_info中
        if (!result.ParseFromHttpResponse(resp_headers, resp_body)) {
            result.SetErrorInfo(resp_body);
        }
    } else {
        result.SetSucc();
        resp->ParseFromXmlString(resp_body);
        resp->ParseFromHeaders(resp_headers);
        resp->SetBody(resp_body);
    }

    return result;
}

CosResult BaseOp::DownloadAction(const std::string& host,
                                 const std::string& path,
                                 const BaseReq& req,
                                 BaseResp* resp,
                                 std::ostream& os) {
    CosResult result;
    std::map<std::string, std::string> req_headers = req.GetHeaders();
    std::map<std::string, std::string> req_params = req.GetParams();

    // 1. 获取host
    req_headers["Host"] = host;

    // 2. 计算签名
    std::string auth_str = AuthTool::Sign(GetAccessKey(), GetSecretKey(),
                                          req.GetMethod(), req.GetPath(),
                                          req_headers, req_params);
    if (auth_str.empty()) {
        result.SetErrorInfo("Generate auth str fail, check your access_key/secret_key.");
        return result;
    }
    req_headers["Authorization"] = auth_str;

    // 3. 发送请求
    std::map<std::string, std::string> resp_headers;
    std::string xml_err_str; // 发送失败返回的xml写入该字符串，避免直接输出到流中

    std::string dest_url = GetRealUrl(host, path);
    int http_code = HttpSender::SendRequest(req.GetMethod(), dest_url, req_params, req_headers,
                                            "", req.GetConnTimeoutInms(), req.GetRecvTimeoutInms(),
                                            &resp_headers, &xml_err_str, os);

    // 4. 解析返回的xml字符串
    result.SetHttpStatus(http_code);
    if (http_code != 200 && http_code != 206) {
        // 无法解析的错误, 填充到cos_result的error_info中
        if (!result.ParseFromHttpResponse(resp_headers, xml_err_str)) {
            result.SetErrorInfo(xml_err_str);
        }
    } else {
        result.SetSucc();
        resp->ParseFromHeaders(resp_headers);
    }

    return result;
}

// TODO(sevenyou) 冗余代码
CosResult BaseOp::UploadAction(const std::string& host,
                               const std::string& path,
                               const BaseReq& req,
                               const std::map<std::string, std::string>& additional_headers,
                               const std::map<std::string, std::string>& additional_params,
                               std::istream& is,
                               BaseResp* resp) {
    CosResult result;
    std::map<std::string, std::string> req_headers = req.GetHeaders();
    std::map<std::string, std::string> req_params = req.GetParams();
    req_headers.insert(additional_headers.begin(), additional_headers.end());
    req_params.insert(additional_params.begin(), additional_params.end());

    // 1. 获取host
    req_headers["Host"] = host;

    // 2. 计算签名
    std::string auth_str = AuthTool::Sign(GetAccessKey(), GetSecretKey(),
                                          req.GetMethod(), req.GetPath(),
                                          req_headers, req_params);
    if (auth_str.empty()) {
        result.SetErrorInfo("Generate auth str fail, check your access_key/secret_key.");
        return result;
    }
    req_headers["Authorization"] = auth_str;

    // 3. 发送请求
    std::map<std::string, std::string> resp_headers;
    std::string resp_body;

    std::string dest_url = GetRealUrl(host, path);
    int http_code = HttpSender::SendRequest(req.GetMethod(), dest_url, req_params, req_headers,
                                            is, req.GetConnTimeoutInms(), req.GetRecvTimeoutInms(),
                                            &resp_headers, &resp_body);

    // 4. 解析返回的xml字符串
    result.SetHttpStatus(http_code);
    if (http_code != 200) {
        // 无法解析的错误, 填充到cos_result的error_info中
        if (!result.ParseFromHttpResponse(resp_headers, resp_body)) {
            result.SetErrorInfo(resp_body);
        }
    } else {
        result.SetSucc();
        resp->ParseFromXmlString(resp_body);
        resp->ParseFromHeaders(resp_headers);
        resp->SetBody(resp_body);
    }

    return result;
}

// 如果设置了目的url, 那么就用设置的, 否则使用appid和bucket拼接的泛域名
std::string BaseOp::GetRealUrl(const std::string& host, const std::string& path) {
    std::string temp = path;
    if (temp.empty() || '/' != temp[0]) {
        temp = "/" + temp;
    }

    if (!CosSysConfig::GetDestDomain().empty()) {
        return "http://" + CosSysConfig::GetDestDomain() + temp;
    }

    return "http://" + host + temp;
}

} // namespace qcloud_cos
