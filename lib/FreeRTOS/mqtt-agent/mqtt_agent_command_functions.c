/*
 * FreeRTOS V202011.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://aws.amazon.com/freertos
 *
 */

/**
 * @file mqtt_agent_command_functions.c
 * @brief Implements functions to process MQTT agent commands.
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* MQTT agent include. */
#include "mqtt_agent.h"

/* Header include. */
#include "mqtt_agent_command_functions.h"

/*-----------------------------------------------------------*/

static MQTTStatus_t runProcessLoops( MQTTAgentContext_t * pMqttAgentContext )
{
    MQTTStatus_t ret = MQTTSuccess;
    const uint32_t processLoopTimeoutMs = 0;

    assert( pMqttAgentContext != NULL );

    do
    {
        pMqttAgentContext->packetReceivedInLoop = false;

        if( ( ret == MQTTSuccess ) &&
            ( pMqttAgentContext->mqttContext.connectStatus == MQTTConnected ) )
        {
            ret = MQTT_ProcessLoop( &( pMqttAgentContext->mqttContext ), processLoopTimeoutMs );
        }
    } while( pMqttAgentContext->packetReceivedInLoop );

    return ret;
}



/*-----------------------------------------------------------*/

MQTTStatus_t MQTTAgentCommand_ProcessLoop( MQTTAgentContext_t * pMqttAgentContext,
                                           void * pUnusedArg,
                                           MQTTAgentCommandFuncReturns_t * pReturnFlags )
{
    ( void ) pUnusedArg;
    assert( pReturnFlags != NULL );

    memset( pReturnFlags, 0x00, sizeof( MQTTAgentCommandFuncReturns_t ) );

    return runProcessLoops( pMqttAgentContext );
}

/*-----------------------------------------------------------*/

MQTTStatus_t MQTTAgentCommand_Publish( MQTTAgentContext_t * pMqttAgentContext,
                                       void * pPublishArg,
                                       MQTTAgentCommandFuncReturns_t * pReturnFlags )
{
    MQTTPublishInfo_t * pPublishInfo;
    MQTTStatus_t ret;
    uint16_t packetId;

    assert( pMqttAgentContext != NULL );
    assert( pPublishArg != NULL );
    assert( pReturnFlags != NULL );

    pPublishInfo = ( MQTTPublishInfo_t * ) ( pPublishArg );

    if( pPublishInfo->qos != MQTTQoS0 )
    {
        pReturnFlags->packetId = MQTT_GetPacketId( &( pMqttAgentContext->mqttContext ) );
    }

    LogInfo( ( "Publishing message to %.*s.\n", ( int ) pPublishInfo->topicNameLength, pPublishInfo->pTopicName ) );
    ret = MQTT_Publish( &( pMqttAgentContext->mqttContext ), pPublishInfo, pReturnFlags->packetId );

    memset( pReturnFlags, 0x00, sizeof( MQTTAgentCommandFuncReturns_t ) );

    /* Add to pending ack list, or call callback if QoS 0. */
    pReturnFlags->addAcknowledgment = ( pPublishInfo->qos != MQTTQoS0 ) && ( ret == MQTTSuccess );

    if( ret == MQTTSuccess )
    {
        ret = runProcessLoops( pMqttAgentContext );

        if( ret != MQTTSuccess )
        {
            pReturnFlags->addAcknowledgment = false;
        }
    }

    return ret;
}

/*-----------------------------------------------------------*/

MQTTStatus_t MQTTAgentCommand_Subscribe( MQTTAgentContext_t * pMqttAgentContext,
                                         void * pSubscribeArgs,
                                         MQTTAgentCommandFuncReturns_t * pReturnFlags )
{
    MQTTAgentSubscribeArgs_t * pSubscribeInfo;
    MQTTStatus_t ret;
    uint16_t packetId;

    assert( pMqttAgentContext != NULL );
    assert( pSubscribeArgs != NULL );
    assert( pReturnFlags != NULL );

    pSubscribeInfo = ( MQTTAgentSubscribeArgs_t * ) ( pSubscribeArgs );
    pReturnFlags->packetId = MQTT_GetPacketId( &( pMqttAgentContext->mqttContext ) );

    ret = MQTT_Subscribe( &( pMqttAgentContext->mqttContext ),
                          pSubscribeInfo->pSubscribeInfo,
                          pSubscribeInfo->numSubscriptions,
                          pReturnFlags->packetId );

    memset( pReturnFlags, 0x00, sizeof( MQTTAgentCommandFuncReturns_t ) );

    if( ret == MQTTSuccess )
    {
        pReturnFlags->addAcknowledgment = true;
        ret = runProcessLoops( pMqttAgentContext );

        if( ret != MQTTSuccess )
        {
            pReturnFlags->addAcknowledgment = false;
        }
    }

    return ret;
}

/*-----------------------------------------------------------*/

MQTTStatus_t MQTTAgentCommand_Unsubscribe( MQTTAgentContext_t * pMqttAgentContext,
                                           void * pSubscribeArgs,
                                           MQTTAgentCommandFuncReturns_t * pReturnFlags )
{
    MQTTAgentSubscribeArgs_t * pSubscribeInfo;
    MQTTStatus_t ret;
    uint16_t packetId;

    assert( pMqttAgentContext != NULL );
    assert( pSubscribeArgs != NULL );
    assert( pReturnFlags != NULL );

    pSubscribeInfo = ( MQTTAgentSubscribeArgs_t * ) ( pSubscribeArgs );
    pReturnFlags->packetId = MQTT_GetPacketId( &( pMqttAgentContext->mqttContext ) );

    ret = MQTT_Unsubscribe( &( pMqttAgentContext->mqttContext ),
                            pSubscribeInfo->pSubscribeInfo,
                            pSubscribeInfo->numSubscriptions,
                            pReturnFlags->packetId );

    memset( pReturnFlags, 0x00, sizeof( MQTTAgentCommandFuncReturns_t ) );

    if( ret == MQTTSuccess )
    {
        pReturnFlags->addAcknowledgment = true;
        ret = runProcessLoops( pMqttAgentContext );

        if( ret != MQTTSuccess )
        {
            pReturnFlags->addAcknowledgment = false;
        }
    }

    return ret;
}

/*-----------------------------------------------------------*/

MQTTStatus_t MQTTAgentCommand_Connect( MQTTAgentContext_t * pMqttAgentContext,
                                       void * pConnectArgs,
                                       MQTTAgentCommandFuncReturns_t * pReturnFlags )
{
    MQTTStatus_t ret;
    MQTTAgentConnectArgs_t * pConnectInfo;

    assert( pMqttAgentContext != NULL );
    assert( pConnectArgs != NULL );
    assert( pReturnFlags != NULL );

    pConnectInfo = ( MQTTAgentConnectArgs_t * ) ( pConnectArgs );

    ret = MQTT_Connect( &( pMqttAgentContext->mqttContext ),
                        pConnectInfo->pConnectInfo,
                        pConnectInfo->pWillInfo,
                        pConnectInfo->timeoutMs,
                        &( pConnectInfo->sessionPresent ) );

    memset( pReturnFlags, 0x00, sizeof( MQTTAgentCommandFuncReturns_t ) );

    return ret;
}

/*-----------------------------------------------------------*/

MQTTStatus_t MQTTAgentCommand_Disconnect( MQTTAgentContext_t * pMqttAgentContext,
                                          void * pUnusedArg,
                                          MQTTAgentCommandFuncReturns_t * pReturnFlags )
{
    MQTTStatus_t ret;

    ( void ) pUnusedArg;

    assert( pMqttAgentContext != NULL );
    assert( pReturnFlags != NULL );

    ret = MQTT_Disconnect( &( pMqttAgentContext->mqttContext ) );

    memset( pReturnFlags, 0x00, sizeof( MQTTAgentCommandFuncReturns_t ) );
    pReturnFlags->endLoop = true;

    return ret;
}

/*-----------------------------------------------------------*/

MQTTStatus_t MQTTAgentCommand_Ping( MQTTAgentContext_t * pMqttAgentContext,
                                    void * pUnusedArg,
                                    MQTTAgentCommandFuncReturns_t * pReturnFlags )
{
    MQTTStatus_t ret;

    ( void ) pUnusedArg;

    assert( pMqttAgentContext != NULL );
    assert( pReturnFlags != NULL );

    ret = MQTT_Ping( &( pMqttAgentContext->mqttContext ) );

    memset( pReturnFlags, 0x00, sizeof( MQTTAgentCommandFuncReturns_t ) );

    if( ret == MQTTSuccess )
    {
        ret = runProcessLoops( pMqttAgentContext );
    }

    return ret;
}

/*-----------------------------------------------------------*/

MQTTStatus_t MQTTAgentCommand_Terminate( MQTTAgentContext_t * pMqttAgentContext,
                                         void * pUnusedArg,
                                         MQTTAgentCommandFuncReturns_t * pReturnFlags )
{
    Command_t * pReceivedCommand = NULL;
    bool receivedCommand = false;
    MQTTAgentReturnInfo_t returnInfo = { 0 };

    ( void ) pUnusedArg;

    assert( pMqttAgentContext != NULL );
    assert( pReturnFlags != NULL );

    returnInfo.returnCode = MQTTBadResponse;

    LogInfo( ( "Terminating command loop.\n" ) );
    memset( pReturnFlags, 0x00, sizeof( MQTTAgentCommandFuncReturns_t ) );
    pReturnFlags->endLoop = true;

    /* Cancel all operations waiting in the queue. */
    do
    {
        receivedCommand = Agent_MessageReceive( pMqttAgentContext->pMessageCtx,
                                                &( pReceivedCommand ),
                                                0U );

        if( ( pReceivedCommand != NULL ) &&
            ( pReceivedCommand->pCommandCompleteCallback != NULL ) )
        {
            pReceivedCommand->pCommandCompleteCallback( pReceivedCommand->pCmdContext, &returnInfo );
        }
    } while( receivedCommand );

    return MQTTSuccess;
}
