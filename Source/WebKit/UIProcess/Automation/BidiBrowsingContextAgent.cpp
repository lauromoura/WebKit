/*
 * Copyright (C) 2025 Apple Inc. All rights reserved.
 * Copyright (C) 2025 Microsoft Corporation. All rights reserved.
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

#include "config.h"
#include "BidiBrowsingContextAgent.h"

#if ENABLE(WEBDRIVER_BIDI)

#include "AutomationProtocolObjects.h"
#include "FrameTreeNodeData.h"
#include "Logging.h"
#include "PageLoadState.h"
#include "WebAutomationSession.h"
#include "WebAutomationSessionMacros.h"
#include "WebDriverBidiFrontendDispatchers.h"
#include "WebDriverBidiProtocolObjects.h"
#include "WebFrameProxy.h"
#include "WebPageProxy.h"
#include "WebProcessPool.h"
#include <wtf/Ref.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

using namespace Inspector;
using BrowsingContext = Inspector::Protocol::BidiBrowsingContext::BrowsingContext;
using ReadinessState = Inspector::Protocol::BidiBrowsingContext::ReadinessState;
using PageLoadStrategy = Inspector::Protocol::Automation::PageLoadStrategy;
using UserPromptType = Inspector::Protocol::BidiBrowsingContext::UserPromptType;
using UserPromptHandlerType = Inspector::Protocol::BidiSession::UserPromptHandlerType;

WTF_MAKE_TZONE_ALLOCATED_IMPL(BidiBrowsingContextAgent);

BidiBrowsingContextAgent::BidiBrowsingContextAgent(WebAutomationSession& session, BackendDispatcher& backendDispatcher)
    : m_session(session)
    , m_browsingContextDomainDispatcher(BidiBrowsingContextBackendDispatcher::create(backendDispatcher, this))
{
}

BidiBrowsingContextAgent::~BidiBrowsingContextAgent() = default;

void BidiBrowsingContextAgent::activate(const BrowsingContext& browsingContext, CommandCallback<void>&& callback)
{
    RefPtr session = m_session.get();
    ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!session, InternalError);

    RefPtr webPageProxy = session->webPageProxyForHandle(browsingContext);
    ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!webPageProxy, WindowNotFound);

    // FIXME: detect non-top level browsing contexts, returning `invalid argument`.
    session->switchToBrowsingContext(browsingContext, emptyString(), [callback = WTFMove(callback)](CommandResult<void>&& result) {
        if (!result) {
            callback(makeUnexpected(result.error()));
            return;
        }

        callback({ });
    });
}

void BidiBrowsingContextAgent::close(const BrowsingContext& browsingContext, std::optional<bool>&& optionalPromptUnload, CommandCallback<void>&& callback)
{
    RefPtr session = m_session.get();
    ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!session, InternalError);

    // FIXME: implement `promptUnload` option.
    // FIXME: raise `invalid argument` if `browsingContext` is not a top-level traversable.

    session->closeBrowsingContext(browsingContext);

    callback({ });
}

static constexpr Inspector::Protocol::Automation::BrowsingContextPresentation defaultBrowsingContextPresentation = Inspector::Protocol::Automation::BrowsingContextPresentation::Tab;

static Inspector::Protocol::Automation::BrowsingContextPresentation browsingContextPresentationFromCreateType(Inspector::Protocol::BidiBrowsingContext::CreateType createType)
{
    switch (createType) {
    case Inspector::Protocol::BidiBrowsingContext::CreateType::Tab:
        return Inspector::Protocol::Automation::BrowsingContextPresentation::Tab;
    case Inspector::Protocol::BidiBrowsingContext::CreateType::Window:
        return Inspector::Protocol::Automation::BrowsingContextPresentation::Window;
    }

    ASSERT_NOT_REACHED();
    return defaultBrowsingContextPresentation;
}

void BidiBrowsingContextAgent::create(Inspector::Protocol::BidiBrowsingContext::CreateType createType, const BrowsingContext& optionalReferenceContext, std::optional<bool>&& optionalBackground, const String& optionalUserContext, CommandCallback<BrowsingContext>&& callback)
{
    RefPtr session = m_session.get();
    ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!session, InternalError);

    // FIXME: implement `referenceContext` option.
    // FIXME: implement `background` option.
    // FIXME: implement `userContext` option.

    session->createBrowsingContext(browsingContextPresentationFromCreateType(createType), [callback = WTFMove(callback)](CommandResultOf<BrowsingContext, Inspector::Protocol::Automation::BrowsingContextPresentation>&& result) {
        if (!result) {
            callback(makeUnexpected(result.error()));
            return;
        }

        auto [resultContext, resultPresentation] = WTFMove(result.value());
        callback(WTFMove(resultContext));
    });
}

Protocol::BidiBrowsingContext::BrowsingContext BidiBrowsingContextAgent::getBrowsingContextID(const WebCore::FrameIdentifier& frameID) const
{
    if (RefPtr frame = WebFrameProxy::webFrame(frameID); frame && frame->isMainFrame()) {
        if (auto page = frame->page())
            return m_session->handleForWebPageProxy(*page);
        return WTF::emptyString();
    }
    return m_session->handleForWebFrameID(frameID);
}

Ref<Protocol::BidiBrowsingContext::Info> BidiBrowsingContextAgent::getNavigableInfo(const WebKit::FrameTreeNodeData& tree, std::optional<unsigned> maxDepth, IncludeParentID includeParentID)
{
    // https://w3c.github.io/webdriver-bidi/#get-the-navigable-info

    // FIXME: Properly support different user contexts, which will likely map to different WebAutomationSessions.
    // https://bugs.webkit.org/show_bug.cgi?id=288104

    // FIXME: Support originalOpener attribute.
    // https://w3c.github.io/webdriver-bidi/#original-opener

    RefPtr<JSON::ArrayOf<Inspector::Protocol::BidiBrowsingContext::Info>> childrenInfo;
    if (!maxDepth || *maxDepth) {
        childrenInfo = JSON::ArrayOf<Inspector::Protocol::BidiBrowsingContext::Info>::create();
        auto newDepth = maxDepth ? std::optional<int>(*maxDepth - 1) : std::nullopt;
        for (auto& child : tree.children)
            childrenInfo->addItem(getNavigableInfo(child, newDepth, IncludeParentID::No));
    }

    auto info = Inspector::Protocol::BidiBrowsingContext::Info::create()
        .setContext(getBrowsingContextID(tree.info.frameID))
        .setUrl(tree.info.request.url().string())
        .setClientWindow("placeholder_window"_s)
        .setUserContext("default"_s)
        .setChildren(WTFMove(childrenInfo))
        .setOriginalOpener(std::nullopt)
        .release();

    if (includeParentID == IncludeParentID::Yes) {
        if (tree.info.parentFrameID)
            info->setParent(getBrowsingContextID(tree.info.parentFrameID.value()));
        else
            info->setNullParent();
    }

    return info;
}

// Recursively traverses the frame tree of the given pages, one page at a time.
// We need such recursion because we need to wait for the frame tree of the current page to be fully processed before moving on to the next page.
void BidiBrowsingContextAgent::getNextTree(Vector<Ref<WebPageProxy>>&& pages, Ref<JSON::ArrayOf<Protocol::BidiBrowsingContext::Info>> contexts, std::optional<unsigned> maxDepth, CommandCallback<Ref<JSON::ArrayOf<Protocol::BidiBrowsingContext::Info>>>&& callback)
{
    if (pages.isEmpty()) {
        callback(WTFMove(contexts));
        return;
    }

    Ref webPageProxy = pages.takeLast();
    webPageProxy->getAllFrameTrees([this, pages = WTFMove(pages), contexts = WTFMove(contexts), callback = WTFMove(callback), maxDepth](Vector<WebKit::FrameTreeNodeData>&& trees) mutable {
        for (auto& tree : trees) {
            auto infoTree = getNavigableInfo(tree, maxDepth, IncludeParentID::Yes);
            contexts->addItem(WTFMove(infoTree));
        }
        getNextTree(WTFMove(pages), WTFMove(contexts), maxDepth, WTFMove(callback));
    });
}

std::optional<unsigned> toOptionalUint(const std::optional<double>& value)
{
    if (!value)
        return std::nullopt;
    if (value.value() < 0)
        return std::nullopt;
    if (std::floor(value.value()) != value.value())
        return std::nullopt;
    if (value.value() > std::numeric_limits<unsigned>::max())
        return std::nullopt;
    return std::optional<unsigned>(static_cast<unsigned>(value.value()));
}

void BidiBrowsingContextAgent::getTree(const BrowsingContext& optionalRoot, std::optional<double>&& optionalMaxDepth, CommandCallback<Ref<JSON::ArrayOf<Protocol::BidiBrowsingContext::Info>>>&& callback)
{
    // https://w3c.github.io/webdriver-bidi/#command-browsingContext-getTree
    RefPtr session = m_session.get();
    ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!session, InternalError);

    Vector<Ref<WebPageProxy>> navigablePages;

    for (Ref process : session->protectedProcessPool()->processes()) {
        for (Ref page : process->pages()) {
            if (!page->isControlledByAutomation())
                continue;

            if (!optionalRoot.isEmpty() && session->handleForWebPageProxy(page) != optionalRoot)
                continue;

            navigablePages.append(page);
        }
    }

    auto infos = JSON::ArrayOf<Inspector::Protocol::BidiBrowsingContext::Info>::create();
    getNextTree(WTFMove(navigablePages), WTFMove(infos), toOptionalUint(optionalMaxDepth), [callback = WTFMove(callback)](auto&& result) {
        callback({ { result.value() } });
    });
}

void BidiBrowsingContextAgent::handleUserPrompt(const BrowsingContext& browsingContext, std::optional<bool>&& optionalShouldAccept, const String&, CommandCallback<void>&& callback)
{
    RefPtr session = m_session.get();
    ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!session, InternalError);

    // FIXME: implement `userText` option.

    if (optionalShouldAccept && *optionalShouldAccept) {
        callback(session->acceptCurrentJavaScriptDialog(browsingContext));
        return;
    }

    // FIXME: this should consider the session's user prompt handler. <https://webkit.org/b/291666>
    callback(session->dismissCurrentJavaScriptDialog(browsingContext));
}


// https://www.w3.org/TR/webdriver/#dfn-session-page-load-timeout
static constexpr Seconds defaultPageLoadTimeout = 300_s;
static constexpr ReadinessState defaultReadinessState = ReadinessState::None;

static PageLoadStrategy pageLoadStrategyFromReadinessState(ReadinessState state)
{
    switch (state) {
    case ReadinessState::None:
        return PageLoadStrategy::None;
    case ReadinessState::Interactive:
        return PageLoadStrategy::Eager;
    case ReadinessState::Complete:
        return PageLoadStrategy::Normal;
    }

    ASSERT_NOT_REACHED();
    return PageLoadStrategy::Normal;
}

void BidiBrowsingContextAgent::navigate(const BrowsingContext& browsingContext, const String& url, std::optional<ReadinessState>&& optionalReadinessState, CommandCallbackOf<String, Inspector::Protocol::BidiBrowsingContext::Navigation>&& callback)
{
    RefPtr session = m_session.get();
    ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!session, InternalError);

    auto pageLoadStrategy = pageLoadStrategyFromReadinessState(optionalReadinessState.value_or(defaultReadinessState));
    session->navigateBrowsingContext(browsingContext, url, pageLoadStrategy, defaultPageLoadTimeout.milliseconds(), [url, callback = WTFMove(callback)](CommandResult<void>&& result) {
        if (!result) {
            callback(makeUnexpected(result.error()));
            return;
        }

        // FIXME: keep track of navigation IDs that we hand out.
        callback({ { url, "placeholder_navigation"_s } });
    });
}

void BidiBrowsingContextAgent::reload(const BrowsingContext& browsingContext, std::optional<bool>&& optionalIgnoreCache, std::optional<ReadinessState>&& optionalReadinessState, CommandCallbackOf<String, Inspector::Protocol::BidiBrowsingContext::Navigation>&& callback)
{
    RefPtr session = m_session.get();
    ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!session, InternalError);

    // FIXME: implement `ignoreCache` option.

    auto pageLoadStrategy = pageLoadStrategyFromReadinessState(optionalReadinessState.value_or(defaultReadinessState));
    session->reloadBrowsingContext(browsingContext, pageLoadStrategy, defaultPageLoadTimeout.milliseconds(), [session = WTFMove(session), browsingContext, callback = WTFMove(callback)](CommandResult<void>&& result) {
        if (!result) {
            callback(makeUnexpected(result.error()));
            return;
        }

        ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!session, InternalError);

        RefPtr webPageProxy = session->webPageProxyForHandle(browsingContext);
        ASYNC_FAIL_WITH_PREDEFINED_ERROR_IF(!webPageProxy, WindowNotFound);

        // FIXME: keep track of navigation IDs that we hand out.
        callback({ { webPageProxy->currentURL(), "placeholder_navigation"_s } });
    });
}

} // namespace WebKit

#endif // ENABLE(WEBDRIVER_BIDI)

