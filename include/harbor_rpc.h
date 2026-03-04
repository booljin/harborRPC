/**
 * @file harbor_rpc.h
 * @brief harborRPC 框架主入口文件
 * 
 * harborRPC是一个用于线程间通信的轻量级RPC框架。
 * RPC调用的本质是将传统的C-S访问结构封装起来，看起来像是一个本地调用。
 * 整个系统包含Client和Service两个部分。
 */

#ifndef __HARBOR_RPC_H__
#define __HARBOR_RPC_H__

#include "harbor_rpc/defines.h"
#include "harbor_rpc/client.h"
#include "harbor_rpc/server.h"
#include "harbor_rpc/harbor.h"
#include "harbor_rpc/util.h"

#endif//__HARBOR_RPC_H__