/*
 * Copyright (C) 2024 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CommandResult.h"
#include "HTTPServer.h"
#include <map>
#include <optional>
#include <vector>
#include <wtf/JSONValues.h>
#include <wtf/text/WTFString.h>

#if USE(SOUP)
#include <wtf/glib/GRefPtr.h>
typedef struct _SoupServer SoupServer;
typedef struct _SoupWebsocketConnection SoupWebsocketConnection;
#endif

namespace WebDriver {

class Session;
class WebDriverService;

struct Command {
    // Defined as js-uint, but in practice opaque for the remote end
    String id;
    String method;
    RefPtr<JSON::Object> parameters;

    static std::optional<Command> fromData(const char* data, size_t dataLength);
};

class WebSocketMessageHandler {
public:

#if USE(SOUP)
    typedef GRefPtr<SoupWebsocketConnection> Connection;
#endif

    struct Message {
        // Optional connection, as the message might be a reply to a command not yet associated to a connection
        std::optional<Connection> connection;
        const char* data { nullptr };
        size_t dataLength { 0 };

        Message createReply(const Message&) const;
        static Message fail(CommandResult::ErrorCode, std::optional<String> errorMessage = std::nullopt, std::optional<String> commandId = std::nullopt);
    };

    virtual void handleHandshake(HTTPRequestHandler::Request&&, Function<void(std::optional<HTTPRequestHandler::Response>&& response)>&& replyHandler) = 0;
    virtual void handleMessage(Message&&, Function<void(Message&&)>&& replyHandler) = 0;
    virtual void clientDisconnected(Connection) = 0;
private:
};

class WebSocketServer {
public:
    explicit WebSocketServer(WebSocketMessageHandler&, WebDriverService&);
    ~WebSocketServer() = default;

    // FIXME Add secure flag?
    std::optional<String> listen(const std::optional<String>& host, unsigned port);
    bool isListening();
    void disconnect();

    void addToConnectionsNotAssociatedToASession(WebSocketMessageHandler::Connection);
    bool isConnectionNotAssociatedToSession(WebSocketMessageHandler::Connection&);
    std::optional<RefPtr<Session>> getSessionForConnection(WebSocketMessageHandler::Connection&);
    String startListeningForAWebSocketConnectionGivenSession(String sessionId);
    String constructWebSocketURLForListenerAndSession(String listener, String sessionId);
    String getSessionIDForWebSocketResource(const String& resource);

private:

    WebSocketMessageHandler& m_messageHandler;
    WebDriverService& m_service;
    String m_listenerURL;

    // List of known WebSocket resources
    std::vector<String> m_webSocketResources;
    // FIXME GRefPtr (our Connection type in Soup is failing to be used as a std::map key, so we use a simple pair for now)
    // std::map<WebSocketMessageHandler::Connection, String> m_connectionToSession;
    std::vector<std::pair<WebSocketMessageHandler::Connection, String>> m_connectionToSession;

#if USE(SOUP)
    GRefPtr<SoupServer> m_soupServer;
    std::vector<GRefPtr<SoupWebsocketConnection>> m_connectionsNotAssociatedToASession;
#endif
};

}
