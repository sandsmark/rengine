/*
    Copyright (c) 2015, Gunnar Sletta <gunnar@sletta.org>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "common/common.h"
#include "common/mathtypes.h"

#include <cassert>

RENGINE_BEGIN_NAMESPACE

class Event
{
public:
	enum Type {
        Invalid,
        PointerDown,
		PointerUp,
        PointerMove,
        KeyDown,
        KeyUp
	};

    Event(Type t) : m_type(t) { }
    virtual ~Event() {}

    inline Type type() const { return m_type; }

private:
	Type m_type;
};

class PointerEvent : public Event
{
public:
    PointerEvent(Type t) : Event(t) {
        assert(t == PointerUp
               || t == PointerDown
               || t == PointerMove);
    };

    /*!

        Called by the backend to initialize this object.

     */
    void initialize(vec2 pos) {
        setPosition(pos);
        setPositionInSurface(pos);
    }

    /*!

        Describes the position of the event relative to the surface it is
        being delivered to. This value does not change dependent on where
        in a scene an object is located.

     */
    void setPositionInSurface(vec2 pos) { m_posInSurface = pos; }
    vec2 positionInSurface() const { return m_posInSurface; }


    /*!

        Describes the position in local coordinates, relative to the local
        object/scene or whatever thing it is being delivered to.

     */
    void setPosition(vec2 position) { m_pos = position; }
	vec2 position() const { return m_pos; }


    static PointerEvent *from(Event *e) {
        assert(e->type() == PointerUp || e->type() == PointerDown || e->type() == PointerMove);
        return static_cast<PointerEvent *>(e);
    }

private:
    vec2 m_pos;
    vec2 m_posInSurface;
};

class KeyEvent : public Event
{
public:
    enum Key {
        Key_0,
        Key_1,
        Key_2,
        Key_3,
        Key_4,
        Key_5,
        Key_6,
        Key_7,
        Key_8,
        Key_9,
        Key_A,
        Key_B,
        Key_C,
        Key_D,
        Key_E,
        Key_F,
        Key_G,
        Key_H,
        Key_I,
        Key_J,
        Key_K,
        Key_L,
        Key_M,
        Key_N,
        Key_O,
        Key_P,
        Key_Q,
        Key_R,
        Key_S,
        Key_T,
        Key_U,
        Key_V,
        Key_W,
        Key_X,
        Key_Y,
        Key_Z,
        Key_Up,
        Key_Left,
        Key_Right,
        Key_Down,
        Key_Enter,
        Key_Space,
        Key_Backspace,
        Key_Escape,

        // Also modifiers
        Key_Shift = 0x80,
        Key_Alt = 0x100,
        Key_Ctrl = 0x200
    };

    KeyEvent(Type t) : Event(t) {
        assert(t == KeyDown || t == KeyUp);
    };

    void initialize(const Key keyCode, const int modifiers) {
        m_keyCode = keyCode;
        m_modifiers = modifiers;
    }

    Key keyCode() const {
        return m_keyCode;
    }

    int modifiers() const {
        return m_modifiers;
    }

    static KeyEvent *from(Event *e) {
        assert(e->type() == KeyUp || e->type() == KeyDown);
        return static_cast<KeyEvent *>(e);
    }


private:
    Key m_keyCode;
    int m_modifiers;
};

RENGINE_END_NAMESPACE
