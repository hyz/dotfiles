#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <memory.h>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include "md.h"

//#include "windows.h"
//#include "md/mdspi.h"
//#include "json.h"

using namespace std;
//#pragma warning(disable : 4996)

int requestId=0;
//extern HANDLE g_hEvent;

map<string, ofstream*> g_files;

void dump(char* instrumentId, char* tradingDay, string content)
{
    // check file handle exists
    string basename("/tmp/");
    string filename = basename + instrumentId + "_" + tradingDay + ".txt";
    map<string, ofstream*>::iterator it = g_files.find(filename);
    if (it == g_files.end()) {
        ofstream* s1 = new ofstream(filename.c_str(), ios::out);
        g_files.insert(std::make_pair(filename, s1));
        (*s1) << content;
        s1->flush();
    } else {
        (*(it->second)) << content;
        it->second->flush();
    }
}

void CtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
        int nRequestID, bool bIsLast)
{
    IsErrorRspInfo(pRspInfo);
}

void CtpMdSpi::OnFrontDisconnected(int nReason)
{
    cerr<<" 响应 | 连接中断..."
        << " reason=" << nReason << endl;
}

void CtpMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
    cerr<<" 响应 | 心跳超时警告..."
        << " TimerLapse = " << nTimeLapse << endl;
}

void CtpMdSpi::OnFrontConnected()
{
    cerr<<" 连接交易前置...成功"<<endl;
    //SetEvent(g_hEvent);
}

void CtpMdSpi::ReqUserLogin(TThostFtdcBrokerIDType	appId,
        TThostFtdcUserIDType	userId,	TThostFtdcPasswordType	passwd)
{
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, appId);
    strcpy(req.UserID, userId);
    strcpy(req.Password, passwd);
    int ret = pUserApi->ReqUserLogin(&req, ++requestId);
    cerr<<" 请求 | 发送登录..."<<((ret == 0) ? "成功" :"失败") << endl;
    //SetEvent(g_hEvent);
}

void CtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (!IsErrorRspInfo(pRspInfo) && pRspUserLogin)
    {
        cerr<<" 响应 | 登录成功...当前交易日:"
            <<pRspUserLogin->TradingDay<<endl;
    }
    //if(bIsLast) SetEvent(g_hEvent);
}

char  instIdList[100];

void CtpMdSpi::SubscribeMarketData(char* instIdList)
{
    vector<char*> list;
    char *token = strtok(instIdList, ",");
    while( token != NULL ){
        list.push_back(token);
        token = strtok(NULL, ",");
    }
    unsigned int len = list.size();
    char** pInstId = new char* [len];
    for(unsigned int i=0; i<len;i++)  pInstId[i]=list[i];
    int ret=pUserApi->SubscribeMarketData(pInstId, len);
    cerr<<" 请求 | 发送行情订阅... "<<((ret == 0) ? "成功" : "失败")<< endl;
    //SetEvent(g_hEvent);
}

void CtpMdSpi::OnRspSubMarketData(
        CThostFtdcSpecificInstrumentField *pSpecificInstrument,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr<<" 响应 |  行情订阅...成功"<<endl;
    //if(bIsLast)  SetEvent(g_hEvent);
}

void CtpMdSpi::OnRspUnSubMarketData(
        CThostFtdcSpecificInstrumentField *pSpecificInstrument,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr<<" 响应 |  行情取消订阅...成功"<<endl;
    //if(bIsLast)  SetEvent(g_hEvent);
}

//std::string serializeMarketDataJson(CThostFtdcDepthMarketDataField *pDepthMarketData)
//{
//	std::string result;
//	JsonObject obj;
//	// strings
//	obj.insert(JsonString("instrumentId"), JsonString(pDepthMarketData->InstrumentID));
//	obj.insert(JsonString("tradingDay"), JsonString(pDepthMarketData->TradingDay));
//	obj.insert(JsonString("updateTime"), JsonString(pDepthMarketData->UpdateTime));
//	obj.insert(JsonString("updateMillisec"), JsonNumber<int>(pDepthMarketData->UpdateMillisec));
//	// exchange info
//	obj.insert(JsonString("exchangeId"), JsonString(pDepthMarketData->ExchangeID));
//	obj.insert(JsonString("exchangeInstId"), JsonString(pDepthMarketData->ExchangeInstID));
//	// infos
//	obj.insert(JsonString("lastPrice"), JsonNumber<double>(pDepthMarketData->LastPrice));
//	obj.insert(JsonString("preSettlementPrice"), JsonNumber<double>(pDepthMarketData->PreSettlementPrice));
//	obj.insert(JsonString("preClosePrice"), JsonNumber<double>(pDepthMarketData->PreClosePrice));
//	obj.insert(JsonString("preOpenInterest"), JsonNumber<double>(pDepthMarketData->PreOpenInterest));
//	obj.insert(JsonString("openPrice"), JsonNumber<double>(pDepthMarketData->OpenPrice));
//	obj.insert(JsonString("highestPrice"), JsonNumber<double>(pDepthMarketData->HighestPrice));
//	obj.insert(JsonString("lowestPrice"), JsonNumber<double>(pDepthMarketData->LowestPrice));
//	obj.insert(JsonString("volume"), JsonNumber<int>(pDepthMarketData->Volume));
//	obj.insert(JsonString("turnover"), JsonNumber<double>(pDepthMarketData->Turnover));
//	obj.insert(JsonString("openInterest"), JsonNumber<double>(pDepthMarketData->OpenInterest));
//	obj.insert(JsonString("closePrice"), JsonNumber<double>(pDepthMarketData->ClosePrice));
//	obj.insert(JsonString("settlementPrice"), JsonNumber<double>(pDepthMarketData->SettlementPrice));
//	obj.insert(JsonString("upperLimitPrice"), JsonNumber<double>(pDepthMarketData->UpperLimitPrice));
//	obj.insert(JsonString("lowerLimitPrice"), JsonNumber<double>(pDepthMarketData->LowerLimitPrice));
//	obj.insert(JsonString("preDelta"), JsonNumber<double>(pDepthMarketData->PreDelta));
//	obj.insert(JsonString("currDelta"), JsonNumber<double>(pDepthMarketData->CurrDelta));
//  // prices
//	obj.insert(JsonString("bidPrice1"), JsonNumber<double>(pDepthMarketData->BidPrice1));
//	obj.insert(JsonString("bidVolume1"), JsonNumber<int>(pDepthMarketData->BidVolume1));
//	obj.insert(JsonString("askPrice1"), JsonNumber<double>(pDepthMarketData->AskPrice1));
//	obj.insert(JsonString("askVolume1"), JsonNumber<int>(pDepthMarketData->AskVolume1));
//	obj.insert(JsonString("bidPrice2"), JsonNumber<double>(pDepthMarketData->BidPrice2));
//	obj.insert(JsonString("bidVolume2"), JsonNumber<int>(pDepthMarketData->BidVolume2));
//	obj.insert(JsonString("askPrice2"), JsonNumber<double>(pDepthMarketData->AskPrice2));
//	obj.insert(JsonString("askVolume2"), JsonNumber<int>(pDepthMarketData->AskVolume2));
//	obj.insert(JsonString("bidPrice3"), JsonNumber<double>(pDepthMarketData->BidPrice3));
//	obj.insert(JsonString("bidVolume3"), JsonNumber<int>(pDepthMarketData->BidVolume3));
//	obj.insert(JsonString("askPrice3"), JsonNumber<double>(pDepthMarketData->AskPrice3));
//	obj.insert(JsonString("askVolume3"), JsonNumber<int>(pDepthMarketData->AskVolume3));
//	obj.insert(JsonString("bidPrice4"), JsonNumber<double>(pDepthMarketData->BidPrice4));
//	obj.insert(JsonString("bidVolume4"), JsonNumber<int>(pDepthMarketData->BidVolume4));
//	obj.insert(JsonString("askPrice4"), JsonNumber<double>(pDepthMarketData->AskPrice4));
//	obj.insert(JsonString("askVolume4"), JsonNumber<int>(pDepthMarketData->AskVolume4));
//	obj.insert(JsonString("bidPrice5"), JsonNumber<double>(pDepthMarketData->BidPrice5));
//	obj.insert(JsonString("bidVolume5"), JsonNumber<int>(pDepthMarketData->BidVolume5));
//	obj.insert(JsonString("askPrice5"), JsonNumber<double>(pDepthMarketData->AskPrice5));
//	obj.insert(JsonString("askVolume5"), JsonNumber<int>(pDepthMarketData->AskVolume5));
//	//
//	obj.insert(JsonString("averagePrice"), JsonNumber<double>(pDepthMarketData->AveragePrice));
//	return obj.serialize() + "\n";
//}

//std::string serializeMarketDataCSV(CThostFtdcDepthMarketDataField *pDepthMarketData)
//{
//	std::stringstream ss;
//	std::string result;
//	ss << pDepthMarketData->InstrumentID << ","
//		 << pDepthMarketData->TradingDay << ","
//		 << pDepthMarketData->UpdateTime << ","
//		 << pDepthMarketData->UpdateMillisec << ","
//		 << pDepthMarketData->LastPrice << ","
//		 << pDepthMarketData->PreSettlementPrice << ","
//		 << pDepthMarketData->PreClosePrice << ","
//		 << pDepthMarketData->PreOpenInterest << ","
//		 << pDepthMarketData->OpenPrice << ","
//		 << pDepthMarketData->HighestPrice << ","
//		 << pDepthMarketData->LowestPrice << ","
//		 << pDepthMarketData->Volume << ","
//		 << pDepthMarketData->Turnover << ","
//		 << pDepthMarketData->OpenInterest << ","
//		 << pDepthMarketData->ClosePrice << ","
//		 << pDepthMarketData->SettlementPrice << ","
//		 << pDepthMarketData->UpperLimitPrice << ","
//		 << pDepthMarketData->LowerLimitPrice << ","
//		 << pDepthMarketData->PreDelta << ","
//		 << pDepthMarketData->CurrDelta << ","
//		 << pDepthMarketData->BidPrice1 << ","
//		 << pDepthMarketData->BidVolume1 << ","
//		 << pDepthMarketData->AskPrice1 << ","
//		 << pDepthMarketData->AskVolume1 << ","
//		 << pDepthMarketData->BidPrice2 << ","
//		 << pDepthMarketData->BidVolume2 << ","
//		 << pDepthMarketData->AskPrice2 << ","
//		 << pDepthMarketData->AskVolume2 << ","
//		 << pDepthMarketData->BidPrice3 << ","
//		 << pDepthMarketData->BidVolume3 << ","
//		 << pDepthMarketData->AskPrice3 << ","
//		 << pDepthMarketData->AskVolume3 << ","
//		 << pDepthMarketData->BidPrice4 << ","
//		 << pDepthMarketData->BidVolume4 << ","
//		 << pDepthMarketData->AskPrice4 << ","
//		 << pDepthMarketData->AskVolume4 << ","
//		 << pDepthMarketData->BidPrice5 << ","
//		 << pDepthMarketData->BidVolume5 << ","
//		 << pDepthMarketData->AskPrice5 << ","
//		 << pDepthMarketData->AskVolume5 << ","
//		 << pDepthMarketData->AveragePrice << ","
//		 << pDepthMarketData->ExchangeID << ","
//		 << pDepthMarketData->ExchangeInstID << ",," << std::endl;
//	return ss.str();
//}

void CtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    cerr<<" 行情 | 合约:"<<pDepthMarketData->InstrumentID
        <<" 日期:"<<pDepthMarketData->TradingDay
        <<" 时间:"<<pDepthMarketData->UpdateTime
        <<" 毫秒:"<<pDepthMarketData->UpdateMillisec
        <<" 现价:"<<pDepthMarketData->LastPrice
        <<" 上次结算价:" << pDepthMarketData->PreSettlementPrice
        <<" 本次结算价:" << pDepthMarketData->SettlementPrice
        <<" 最高价:" << pDepthMarketData->HighestPrice
        <<" 最低价:" << pDepthMarketData->LowestPrice
        <<" 数量:" << pDepthMarketData->Volume
        <<" 成交金额:" << pDepthMarketData->Turnover
        <<" 持仓量:" << pDepthMarketData->OpenInterest
        <<" 卖一价:" << pDepthMarketData->AskPrice1
        <<" 卖一量:" << pDepthMarketData->AskVolume1
        <<" 买一价:" << pDepthMarketData->BidPrice1
        <<" 买一量:" << pDepthMarketData->BidVolume1
        <<" 持仓量:"<< pDepthMarketData->OpenInterest <<endl;
    //cerr << "csv: " << serializeMarketDataCSV(pDepthMarketData) << endl;
    //cerr << "json: " << serializeMarketDataJson(pDepthMarketData) << endl;
    //dump(pDepthMarketData->InstrumentID, pDepthMarketData->TradingDay, serializeMarketDataJson(pDepthMarketData));
}

bool CtpMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    bool ret = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (ret){
        cerr<<" 响应 | "<<pRspInfo->ErrorMsg<<endl;
    }
    return ret;
}

//#include <iostream>
//void ShowTraderCommand(CtpTraderSpi* p, bool print=false)
//{
//  if(print){
//    cerr<<"-----------------------------------------------"<<endl;
//    cerr<<" [1] ReqUserLogin              -- 登录"<<endl;
//    cerr<<" [2] ReqSettlementInfoConfirm  -- 结算单确认"<<endl;
//    cerr<<" [3] ReqQryInstrument          -- 查询合约"<<endl;
//    cerr<<" [4] ReqQryTradingAccount      -- 查询资金"<<endl;
//    cerr<<" [5] ReqQryInvestorPosition    -- 查询持仓"<<endl;
//    cerr<<" [6] ReqOrderInsert            -- 报单"<<endl;
//    cerr<<" [7] ReqOrderAction            -- 撤单"<<endl;
//    cerr<<" [8] PrintOrders               -- 显示报单"<<endl;
//    cerr<<" [9] PrintOrders               -- 显示成交"<<endl;
//    cerr<<" [0] Exit                      -- 退出"<<endl;
//    cerr<<"----------------------------------------------"<<endl;
//  }
//  TThostFtdcBrokerIDType	appId;
//	TThostFtdcUserIDType	userId;
//	TThostFtdcPasswordType	passwd;
//  TThostFtdcInstrumentIDType instId;
//  TThostFtdcDirectionType dir;
//  TThostFtdcCombOffsetFlagType kpp;
//  TThostFtdcPriceType price;
//  TThostFtdcVolumeType vol;
//  TThostFtdcSequenceNoType orderSeq;
//
//  int cmd;  cin>>cmd;
//  switch(cmd){
//    case 1: {
//              cerr<<" 应用单元 > ";cin>>appId;
//              cerr<<" 投资者代码 > ";cin>>userId;
//              cerr<<" 交易密码 > ";cin>>passwd;
//              p->ReqUserLogin(appId,userId,passwd); break;
//            }
//    case 2: p->ReqSettlementInfoConfirm(); break;
//    case 3: {
//              cerr<<" 合约 > "; cin>>instId;
//              p->ReqQryInstrument(instId); break;
//            }
//    case 4: p->ReqQryTradingAccount(); break;
//    case 5: {
//              cerr<<" 合约 > "; cin>>instId;
//              p->ReqQryInvestorPosition(instId); break;
//            }
//    case 6: {
//              cerr<<" 合约 > "; cin>>instId;
//              cerr<<" 方向 > "; cin>>dir;
//              cerr<<" 开平 > "; cin>>kpp;
//              cerr<<" 价格 > "; cin>>price;
//              cerr<<" 数量 > "; cin>>vol;
//              p->ReqOrderInsert(instId,dir,kpp,price,vol); break;
//            }
//    case 7: {
//              cerr<<" 序号 > "; cin>>orderSeq;
//              p->ReqOrderAction(orderSeq);break;
//            }
//    case 8: p->PrintOrders();break;
//    case 9: p->PrintTrades();break;
//    case 0: exit(0);
//  }
//  //WaitForSingleObject(g_hEvent,INFINITE);
//  //ResetEvent(g_hEvent);
//  ShowTraderCommand(p);
//}

//void ShowMdCommand(CtpMdSpi* p, bool print=false)
//{
//  if(print){
//    cerr<<"-----------------------------------------------"<<endl;
//    cerr<<" [1] ReqUserLogin              -- 登录"<<endl;
//    cerr<<" [2] SubscribeMarketData       -- 行情订阅"<<endl;
//    cerr<<" [3] "<<endl;
//    cerr<<" [0] Exit                      -- 退出"<<endl;
//    cerr<<"----------------------------------------------"<<endl;
//  }
//  TThostFtdcBrokerIDType appId = "88888";
//	TThostFtdcUserIDType	 userId = "sywg";
//	TThostFtdcPasswordType passwd = "8888";
//
//  int cmd;  cin>>cmd;
//  switch(cmd){
//    case 1: {
//                strcpy(appId, "88888");
//                strcpy(userId, "sywg");
//                strcpy(passwd, "8888");
//              //cerr<<" 应用单元 > ";cin>>appId;
//              //cerr<<" 投资者代码 > ";cin>>userId;
//              //cerr<<" 交易密码 > ";cin>>passwd;
//              p->ReqUserLogin(appId,userId,passwd); break;
//            }
//    case 2: {
//              strcpy(instIdList, "ag1606");
//              //cerr<<" 合约 > "; cin>>instIdList;
//              p->SubscribeMarketData(instIdList); break;
//            }
//    case 3: {
//              cerr << "自动开始 ";
//              strcpy(appId, "1");
//              strcpy(userId, "1");
//              strcpy(passwd, "1");
//              sleep(3);
//              p->ReqUserLogin(appId,userId,passwd);
//              sleep(1);
//              strcpy(instIdList,"IF1503,IF1504,IF1506,IF1509");
//              p->SubscribeMarketData(instIdList);
//              break;
//            }
//    case 0: exit(0);
//  }
//  //WaitForSingleObject(g_hEvent,INFINITE);
//  //ResetEvent(g_hEvent);
//  ShowMdCommand(p);
//}

//void ShowRiskCommand(CtpRiskSpi* p, bool print=false)
//{
//  if(print){
//    cerr<<"-----------------------------------------------"<<endl;
//    cerr<<" [1] ReqUserLogin              -- 登录"<<endl;
//    cerr<<" [2] ReqRiskQryBrokerDeposit   -- 查询席位资金"<<endl;
//    cerr<<" [3] ReqRiskOrderInsert        -- 强平"<<endl;
//    cerr<<" [4] PrintAcounts              -- 显示资金账户"<<endl;
//    cerr<<" [5] PrintPositions            -- 显示持仓信息"<<endl;
//    cerr<<" [0] Exit                      -- 退出"<<endl;
//    cerr<<"----------------------------------------------"<<endl;
//  }
//
//  TShfeFtdcBrokerIDType	appId;
//  TShfeFtdcUserIDType	 userId;
//  TShfeFtdcPasswordType	passwd;
//  TShfeFtdcExchangeIDType exchangeId;
//  TShfeFtdcSequenceNoType seqNo;
//  TShfeFtdcPriceType price;
//  int cmd;  cin>>cmd;
//  switch(cmd){
//    case 1: {
//              cerr<<" 应用单元 > ";cin>>appId;
//              cerr<<" 操作员 > ";cin>>userId;
//              cerr<<" 密码 > ";cin>>passwd;
//              p->ReqUserLogin(appId,userId,passwd); break;
//            }
//    case 2: {
//              cerr<<" 交易所 > ";cin>>exchangeId;
//              p->ReqRiskQryBrokerDeposit(exchangeId);break;
//            }
//    case 3: {
//              cerr<<" 序号 > ";cin>>seqNo;
//              cerr<<" 价格 > ";cin>>price;
//              p->ReqRiskOrderInsert(seqNo,price);break;
//            }
//    case 4: p->PrintAcounts(); break;
//    case 5: p->PrintPositions();break;
//    case 0: exit(0);
//  }
//  //WaitForSingleObject(g_hEvent,INFINITE);
//  //ResetEvent(g_hEvent);
//  ShowRiskCommand(p);
//}

//#pragma once
TThostFtdcBrokerIDType appId;		// 应用单元
TThostFtdcUserIDType userId;		// 投资者代码

// 会话参数
char orderRef[13];

vector<CThostFtdcOrderField*> orderList;
vector<CThostFtdcTradeField*> tradeList;

char MapDirection(char src, bool toOrig=true){
    if(toOrig){
        if('b'==src||'B'==src){src='0';}else if('s'==src||'S'==src){src='1';}
    }else{
        if('0'==src){src='B';}else if('1'==src){src='S';}
    }
    return src;
}
char MapOffset(char src, bool toOrig=true){
    if(toOrig){
        if('o'==src||'O'==src){src='0';}
        else if('c'==src||'C'==src){src='1';}
        else if('j'==src||'J'==src){src='3';}
    }else{
        if('0'==src){src='O';}
        else if('1'==src){src='C';}
        else if('3'==src){src='J';}
    }
    return src;
}

void CtpTraderSpi::OnFrontConnected()
{
    cerr<<" 连接交易前置...成功"<<endl;
    //SetEvent(g_hEvent);
}

void CtpTraderSpi::ReqUserLogin(TThostFtdcBrokerIDType	vAppId,
        TThostFtdcUserIDType	vUserId,	TThostFtdcPasswordType	vPasswd)
{
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, vAppId); strcpy(appId, vAppId);
    strcpy(req.UserID, vUserId);  strcpy(userId, vUserId);
    strcpy(req.Password, vPasswd);
    int ret = pUserApi->ReqUserLogin(&req, ++requestId);
    cerr<<" 请求 | 发送登录..."<<((ret == 0) ? "成功" :"失败") << endl;
}

void CtpTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if ( !IsErrorRspInfo(pRspInfo) && pRspUserLogin ) {
        // 保存会话参数
        int	 frontId;	//前置编号
        frontId = pRspUserLogin->FrontID;
        int	 sessionId;	//会话编号
        sessionId = pRspUserLogin->SessionID;
        int nextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
        sprintf(orderRef, "%d", ++nextOrderRef);
        cerr<<" 响应 | 登录成功...当前交易日:"
            <<pRspUserLogin->TradingDay<<endl;
    }
    //if(bIsLast) SetEvent(g_hEvent);
}

void CtpTraderSpi::ReqSettlementInfoConfirm()
{
    CThostFtdcSettlementInfoConfirmField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, appId);
    strcpy(req.InvestorID, userId);
    int ret = pUserApi->ReqSettlementInfoConfirm(&req, ++requestId);
    cerr<<" 请求 | 发送结算单确认..."<<((ret == 0)?"成功":"失败")<<endl;
}

void CtpTraderSpi::OnRspSettlementInfoConfirm(
        CThostFtdcSettlementInfoConfirmField  *pSettlementInfoConfirm,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if( !IsErrorRspInfo(pRspInfo) && pSettlementInfoConfirm){
        cerr<<" 响应 | 结算单..."<<pSettlementInfoConfirm->InvestorID
            <<"...<"<<pSettlementInfoConfirm->ConfirmDate
            <<" "<<pSettlementInfoConfirm->ConfirmTime<<">...确认"<<endl;
    }
    //if(bIsLast) SetEvent(g_hEvent);
}

void CtpTraderSpi::ReqQryInstrument(TThostFtdcInstrumentIDType instId)
{
    CThostFtdcQryInstrumentField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.InstrumentID, instId);
    int ret = pUserApi->ReqQryInstrument(&req, ++requestId);
    cerr<<" 请求 | 发送合约查询..."<<((ret == 0)?"成功":"失败")<<endl;
}

void CtpTraderSpi::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if ( !IsErrorRspInfo(pRspInfo) &&  pExchange){
        cerr<<" 响应 | 交易所:"<<pExchange->ExchangeID
            <<" 名称:"<<pExchange->ExchangeName
            <<" 属性:"<<int(pExchange->ExchangeProperty)
            <<endl;
    }
}

void CtpTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if ( !IsErrorRspInfo(pRspInfo) &&  pInstrument){
        cerr<<" 响应 | 合约:"<<pInstrument->InstrumentID
            <<" 交割月:"<<pInstrument->DeliveryMonth
            <<" 多头保证金率:"<<pInstrument->LongMarginRatio
            <<" 空头保证金率:"<<pInstrument->ShortMarginRatio<<endl;
    }
    //if(bIsLast) SetEvent(g_hEvent);
}

void CtpTraderSpi::ReqQryTradingAccount()
{
    CThostFtdcQryTradingAccountField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, appId);
    strcpy(req.InvestorID, userId);
    int ret = pUserApi->ReqQryTradingAccount(&req, ++requestId);
    cerr<<" 请求 | 发送资金查询..."<<((ret == 0)?"成功":"失败")<<endl;

}

void CtpTraderSpi::OnRspQryTradingAccount(
        CThostFtdcTradingAccountField *pTradingAccount,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (!IsErrorRspInfo(pRspInfo) &&  pTradingAccount){
        cerr<<" 响应 | 权益:"<<pTradingAccount->Balance
            <<" 可用:"<<pTradingAccount->Available
            <<" 保证金:"<<pTradingAccount->CurrMargin
            <<" 平仓盈亏:"<<pTradingAccount->CloseProfit
            <<" 持仓盈亏"<<pTradingAccount->PositionProfit
            <<" 手续费:"<<pTradingAccount->Commission
            <<" 冻结保证金:"<<pTradingAccount->FrozenMargin
            //<<" 冻结手续费:"<<pTradingAccount->FrozenCommission
            << endl;
    }
    //if(bIsLast) SetEvent(g_hEvent);
}

void CtpTraderSpi::ReqQryInvestorPosition(TThostFtdcInstrumentIDType instId)
{
    CThostFtdcQryInvestorPositionField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, appId);
    strcpy(req.InvestorID, userId);
    strcpy(req.InstrumentID, instId);
    int ret = pUserApi->ReqQryInvestorPosition(&req, ++requestId);
    cerr<<" 请求 | 发送持仓查询..."<<((ret == 0)?"成功":"失败")<<endl;
}

void CtpTraderSpi::OnRspQryInvestorPosition(
        CThostFtdcInvestorPositionField *pInvestorPosition,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if( !IsErrorRspInfo(pRspInfo) &&  pInvestorPosition ){
        cerr<<" 响应 | 合约:"<<pInvestorPosition->InstrumentID
            <<" 方向:"<<MapDirection(pInvestorPosition->PosiDirection-2,false)
            <<" 总持仓:"<<pInvestorPosition->Position
            <<" 昨仓:"<<pInvestorPosition->YdPosition
            <<" 今仓:"<<pInvestorPosition->TodayPosition
            <<" 持仓盈亏:"<<pInvestorPosition->PositionProfit
            <<" 保证金:"<<pInvestorPosition->UseMargin<<endl;
    }
    //if(bIsLast) SetEvent(g_hEvent);
}

void CtpTraderSpi::ReqOrderInsert(TThostFtdcInstrumentIDType instId,
        TThostFtdcDirectionType dir, TThostFtdcCombOffsetFlagType kpp,
        TThostFtdcPriceType price,   TThostFtdcVolumeType vol)
{
    CThostFtdcInputOrderField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, appId);  //应用单元代码
    strcpy(req.InvestorID, userId); //投资者代码
    strcpy(req.InstrumentID, instId); //合约代码
    strcpy(req.OrderRef, orderRef);  //报单引用
    int nextOrderRef = atoi(orderRef);
    sprintf(orderRef, "%d", ++nextOrderRef);

    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;//价格类型=限价
    req.Direction = MapDirection(dir,true);  //买卖方向
    req.CombOffsetFlag[0] = MapOffset(kpp[0],true); //THOST_FTDC_OF_Open; //组合开平标志:开仓
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;	  //组合投机套保标志
    req.LimitPrice = price;	//价格
    req.VolumeTotalOriginal = vol;	///数量
    req.TimeCondition = THOST_FTDC_TC_GFD;  //有效期类型:当日有效
    req.VolumeCondition = THOST_FTDC_VC_AV; //成交量类型:任何数量
    req.MinVolume = 1;	//最小成交量:1
    req.ContingentCondition = THOST_FTDC_CC_Immediately;  //触发条件:立即

    //TThostFtdcPriceType	StopPrice;  //止损价
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;	//强平原因:非强平
    req.IsAutoSuspend = 0;  //自动挂起标志:否
    req.UserForceClose = 0;   //用户强评标志:否

    int ret = pUserApi->ReqOrderInsert(&req, ++requestId);
    cerr<<" 请求 | 发送报单..."<<((ret == 0)?"成功":"失败")<< endl;
}

void CtpTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if( !IsErrorRspInfo(pRspInfo) && pInputOrder ){
        cerr<<"响应 | 报单提交成功...报单引用:"<<pInputOrder->OrderRef<<endl;
    }
    //if(bIsLast) SetEvent(g_hEvent);
}

void CtpTraderSpi::ReqOrderAction(TThostFtdcSequenceNoType orderSeq)
{
    bool found=false; unsigned int i=0;
    for(i=0;i<orderList.size();i++){
        if(orderList[i]->BrokerOrderSeq == orderSeq){ found = true; break;}
    }
    if(!found){cerr<<" 请求 | 报单不存在."<<endl; return;}

    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, appId);   //经纪公司代码
    strcpy(req.InvestorID, userId); //投资者代码
    //strcpy(req.OrderRef, pOrderRef); //报单引用
    //req.FrontID = frontId;           //前置编号
    //req.SessionID = sessionId;       //会话编号
    strcpy(req.ExchangeID, orderList[i]->ExchangeID);
    strcpy(req.OrderSysID, orderList[i]->OrderSysID);
    req.ActionFlag = THOST_FTDC_AF_Delete;  //操作标志

    int ret = pUserApi->ReqOrderAction(&req, ++requestId);
    cerr<< " 请求 | 发送撤单..." <<((ret == 0)?"成功":"失败") << endl;
}

void CtpTraderSpi::OnRspOrderAction(
        CThostFtdcInputOrderActionField *pInputOrderAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (!IsErrorRspInfo(pRspInfo) && pInputOrderAction){
        cerr<< " 响应 | 撤单成功..."
            << "交易所:"<<pInputOrderAction->ExchangeID
            <<" 报单编号:"<<pInputOrderAction->OrderSysID<<endl;
    }
    //if(bIsLast) SetEvent(g_hEvent);
}

///报单回报
void CtpTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    CThostFtdcOrderField* order = new CThostFtdcOrderField();
    memcpy(order,  pOrder, sizeof(CThostFtdcOrderField));
    bool founded=false;    unsigned int i=0;
    for(i=0; i<orderList.size(); i++){
        if(orderList[i]->BrokerOrderSeq == order->BrokerOrderSeq) {
            founded=true;    break;
        }
    }
    if(founded) orderList[i]= order;
    else  orderList.push_back(order);
    cerr<<" 回报 | 报单已提交...序号:"<<order->BrokerOrderSeq<<endl;
    //SetEvent(g_hEvent);
}

///成交通知
void CtpTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    CThostFtdcTradeField* trade = new CThostFtdcTradeField();
    memcpy(trade,  pTrade, sizeof(CThostFtdcTradeField));
    bool founded=false;     unsigned int i=0;
    for(i=0; i<tradeList.size(); i++){
        if(tradeList[i]->TradeID == trade->TradeID) {
            founded=true;   break;
        }
    }
    if(founded) tradeList[i] = trade;
    else  tradeList.push_back(trade);
    cerr<<" 回报 | 报单已成交...成交编号:"<<trade->TradeID<<endl;
    //SetEvent(g_hEvent);
}

void CtpTraderSpi::OnFrontDisconnected(int nReason)
{
    cerr<<" 响应 | 连接中断..."
        << " reason=" << nReason << endl;
}

void CtpTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
    cerr<<" 响应 | 心跳超时警告..."
        << " TimerLapse = " << nTimeLapse << endl;
}

void CtpTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    IsErrorRspInfo(pRspInfo);
}

bool CtpTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    // 如果ErrorID != 0, 说明收到了错误的响应
    bool ret = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (ret){
        cerr<<" 响应 | "<<pRspInfo->ErrorMsg<<endl;
    }
    return ret;
}

void CtpTraderSpi::PrintOrders(){
    CThostFtdcOrderField* pOrder;
    for(unsigned int i=0; i<orderList.size(); i++){
        pOrder = orderList[i];
        cerr<<" 报单 | 合约:"<<pOrder->InstrumentID
            <<" 方向:"<<MapDirection(pOrder->Direction,false)
            <<" 开平:"<<MapOffset(pOrder->CombOffsetFlag[0],false)
            <<" 价格:"<<pOrder->LimitPrice
            <<" 数量:"<<pOrder->VolumeTotalOriginal
            <<" 序号:"<<pOrder->BrokerOrderSeq
            <<" 报单编号:"<<pOrder->OrderSysID
            <<" 状态:"<<pOrder->StatusMsg<<endl;
    }
    //SetEvent(g_hEvent);
}
void CtpTraderSpi::PrintTrades(){
    CThostFtdcTradeField* pTrade;
    for(unsigned int i=0; i<tradeList.size(); i++){
        pTrade = tradeList[i];
        cerr<<" 成交 | 合约:"<< pTrade->InstrumentID
            <<" 方向:"<<MapDirection(pTrade->Direction,false)
            <<" 开平:"<<MapOffset(pTrade->OffsetFlag,false)
            <<" 价格:"<<pTrade->Price
            <<" 数量:"<<pTrade->Volume
            <<" 报单编号:"<<pTrade->OrderSysID
            <<" 成交编号:"<<pTrade->TradeID<<endl;
    }
    //SetEvent(g_hEvent);
}

void test_md(void)
{
    //初始化UserApi
    CThostFtdcMdApi* pUserApi=CThostFtdcMdApi::CreateFtdcMdApi();
    CtpMdSpi* pUserSpi=new CtpMdSpi(pUserApi); //创建回调处理类对象MdSpi
    pUserApi->RegisterSpi(pUserSpi);			// 回调对象注入接口类

    char mdFront[]   ="tcp://asp-sim1-front1.financial-trading-platform.com:41213";
    //strcpy(mdFront, "tcp://shsywg5dx6.pobo.com.cn:13792");
    strcpy(mdFront, "tcp://180.168.212.51:41213");
    pUserApi->RegisterFront(mdFront);		  // 注册行情前置地址

    pUserApi->Init();      //接口线程启动, 开始工作
    //ShowMdCommand(pUserSpi,true); return;

    TThostFtdcBrokerIDType appId;
    TThostFtdcUserIDType	 userId;
    TThostFtdcPasswordType passwd;

    char pause;
    std::cin >> pause;

    strcpy(appId, "88888");
    strcpy(userId, "sywg");
    strcpy(passwd, "8888");
    pUserSpi->ReqUserLogin(appId,userId,passwd);

    std::cin >> pause;

    char  idlist[100];
    strcpy(idlist, "ag1606");
    //cerr<<" 合约 > "; cin>>idlist;
    pUserSpi->SubscribeMarketData(idlist);

    pUserApi->Join();      //等待接口线程退出

    //pUserApi->Release(); //接口对象释放
}

// void test_order(void)
// {
//   char pause;
// 
//   //初始化UserApi
//   CThostFtdcTraderApi* pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
//   CtpTraderSpi* pUserSpi = new CtpTraderSpi(pUserApi);
//   pUserApi->RegisterSpi(/*(CThostFtdcTraderSpi*)*/pUserSpi);			// 注册事件类
//   pUserApi->SubscribePublicTopic(THOST_TERT_RESTART);					// 注册公有流
//   pUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);			  // 注册私有流
// 
// char tradeFront[]="tcp://asp-sim1-front1.financial-trading-platform.com:41205";
//   strcpy(tradeFront, "tcp://180.168.212.51:41205");
//   pUserApi->RegisterFront(tradeFront);							// 注册交易前置地址
// 
//   pUserApi->Init();
//   //=ShowTraderCommand(pUserSpi,true);
// 
//   std::cin >> pause;
// 
//   TThostFtdcBrokerIDType	appId;
// 	TThostFtdcUserIDType	userId;
// 	TThostFtdcPasswordType	passwd;
//               strcpy(appId, "88888");
//               strcpy(userId, "8011005869");
//               strcpy(passwd, "809912");
//   pUserSpi->ReqUserLogin(appId,userId, passwd);
//   std::cin >> pause;
// 
//   pUserSpi->ReqQryTradingAccount();
//   std::cin >> pause;
// 
// CThostFtdcQryExchangeField exf;
// strcpy(exf.ExchangeID, "DCE");
// 	pUserApi->ReqQryExchange(&exf, 123);//( *pQryExchange, int nRequestID) = 0;
// 
//   pUserApi->Join();
//   //pUserApi->Release();
// }


/*
   void test_risk()
   {
   CShfeFtdcRiskUserApi* pUserApi=CShfeFtdcRiskUserApi::CreateFtdcRiskUserApi();
   CtpRiskSpi* pUserSpi=new CtpRiskSpi(pUserApi);
   pUserApi->RegisterSpi(pUserSpi);
   char riskFront[] ="tcp://asp-sim1-front1.financial-trading-platform.com:50001";
   pUserApi->RegisterFront(riskFront);

   pUserApi->Init();
   ShowRiskCommand(pUserSpi,true);
   pUserApi->Join();
//pUserApi->Release();
}
*/

int main(int argc, const char* argv[])
{
    //g_hEvent=CreateEvent(NULL, true, false, NULL);
    if(argc < 2)  cerr<<"格式: 命令 参数, 输入有误."<<endl;
    else if(strcmp(argv[1],"--md")==0)    test_md();
    //else if(strcmp(argv[1],"--order")==0) test_order();
    //else if(strcmp(argv[1],"--risk")==0)  test_risk();
    return 0;
}

