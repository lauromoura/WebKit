/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include "StyleOriginatedAnimationEvent.h"

namespace WebCore {

class CSSAnimationEvent final : public StyleOriginatedAnimationEvent {
    WTF_MAKE_TZONE_OR_ISO_ALLOCATED(CSSAnimationEvent);
public:
    static Ref<CSSAnimationEvent> create(const AtomString& type, WebAnimation* animation, std::optional<Seconds> scheduledTime, double elapsedTime, const std::optional<Style::PseudoElementIdentifier>& pseudoElementIdentifier, const String& animationName)
    {
        return adoptRef(*new CSSAnimationEvent(type, animation, scheduledTime, elapsedTime, pseudoElementIdentifier, animationName));
    }

    struct Init : EventInit {
        String animationName;
        double elapsedTime { 0 };
        String pseudoElement;
    };

    static Ref<CSSAnimationEvent> create(const AtomString& type, const Init& initializer, IsTrusted isTrusted = IsTrusted::No)
    {
        return adoptRef(*new CSSAnimationEvent(type, initializer, isTrusted));
    }

    virtual ~CSSAnimationEvent();

    bool isCSSAnimationEvent() const final { return true; }

    const String& animationName() const { return m_animationName; }

private:
    CSSAnimationEvent(const AtomString& type, WebAnimation*, std::optional<Seconds> scheduledTime, double elapsedTime, const std::optional<Style::PseudoElementIdentifier>&, const String& animationName);
    CSSAnimationEvent(const AtomString&, const Init&, IsTrusted);

    String m_animationName;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_ANIMATION_EVENT_BASE(CSSAnimationEvent, isCSSAnimationEvent())
