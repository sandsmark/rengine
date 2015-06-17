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

#include <assert.h>
#include <vector>
#include <algorithm>
#include <iostream>

RENGINE_BEGIN_NAMESPACE

class Node {
public:
    enum Type {
        BasicNodeType = 0,
        RectangleNodeType,
        LayerNodeType,
        TransformNodeType,
        OpacityNodeType
    };

    /*!
     * Node constructor...
     */
    Node(Type type = BasicNodeType)
        : m_parent(0)
        , m_type(type)
        , m_preprocess(false)
    {
    }

    /*!
     * Node destructor.
     *
     * A Node will delete all its children when the destructor runs.
     */
    virtual ~Node() {
        if (m_parent)
            m_parent->remove(this);
        while (!m_children.empty())
            delete m_children.back();
    }

    /*!
     * Adds \a child at the end of this node's list of children.
     *
     * It is an error to add a child which already has a parent
     * or is already a child of this node.
     */
    void append(Node *child) {
        assert(std::find(m_children.begin(), m_children.end(), child) == m_children.end());
        m_children.push_back(child);
        child->setParent(this);
    }

    Node &operator<<(Node *child) { append(child); return *this; }

    /*!
     * Adds \a child at the front of this node's list of children.
     *
     * It is an error to add a child which already has a parent
     * or is already a child of this node.
     */
    void prepend(Node *child) {
        assert(std::find(m_children.begin(), m_children.end(), child) == m_children.end());
        m_children.insert(m_children.begin(), child);
        child->setParent(this);
    }

    /*!
     * Removes \a child from this node's list of children.
     *
     * It is an error to remove a child which is not a child of
     * this node.
     */
    void remove(Node *child) {
        assert(std::find(m_children.begin(), m_children.end(), child) != m_children.end());
        m_children.erase(std::find(m_children.begin(), m_children.end(), child));
        child->setParent(0);
    }

    /*!
     * Injects this node into the tree above \a node. This ndoe becomes a
     * parent for \a node and will have the same order in the original parent's child list.
     */
    void injectAbove(Node *node) {
        assert(node);
        assert(node->parent());
        assert(!parent());

        // Attach ourselves to the node's parent
        Node *p = node->parent();
        auto pos = std::find(p->m_children.begin(), p->m_children.end(), node);
        assert(pos != p->m_children.end());
        *pos = this;
        m_parent = p;

        // attach node to ourselves
        m_children.push_back(node);
        node->m_parent = this;
    }

    /*!
     * Removes this node from the tree and assigns all of its children to this
     * node's parent.
     */
    void evict() {
        assert(parent());
        Node *p = parent();
        auto pos = std::find(p->m_children.begin(), m_children.end(), this);
        assert(pos != p->m_children.end());
        pos = p->m_children.erase(pos);
        for (auto i : m_children) {
            pos = p->m_children.insert(pos, i);
            i->m_parent = p;
        }
        m_children.clear();
        m_parent = 0;
    }

    /*!
     * Returns the number of children in this node
     */
    int childCount() const { return m_children.size(); }

    /*!
     * Returns this node's list of children.
     */
    const std::vector<Node *> &children() const { return m_children; }

    /*!
     * Returns this node's parent node.
     */
    Node *parent() const { return m_parent; }

    /*!
     * Returns this node's type.
     */
    Type type() const { return m_type; }

    /*!
     * Helper function to downcast from Node * to a specific subclass.
     */
    template <typename T>
    static T* from(Node *n) {
        assert(n);
        return (n->type() == (Node::Type) T::StaticType) ? static_cast<T *>(n) : 0;
    }

    /*!
     * Helper function to downcast from const Node * to a specific subclass.
     */
    template <typename T>
    static const T* from(const Node *n) {
        assert(n);
        return (n->type() == T::StaticType) ? static_cast<const T *>(n) : 0;
    }

    void requestPreprocess() { m_preprocess = true; }
    void preprocess() {
        if (m_preprocess) {
            m_preprocess = false;
            onPreprocess();
        }
    }

    static void dump(Node *n, unsigned level = 0)
    {
        for (unsigned x=0; x<level; ++x) std::cout << " ";
        switch (n->type()) {
        case Node::BasicNodeType: std::cout << "Node"; break;
        case Node::OpacityNodeType: std::cout << "OpacityNode"; break;
        case Node::TransformNodeType: std::cout << "TransormNode"; break;
        case Node::RectangleNodeType: std::cout << "RectangleNode"; break;
        case Node::LayerNodeType: std::cout << "LayerNodeType"; break;
        default: std::cout << "Node(type=" << n->type() << ")"; break;
        }
        std::cout << "(" << n << ") parent=" << n->parent() << " childCount=" << n->childCount() << std::endl;
        for (auto child : n->children())
            dump(child, level + 1);
    }



protected:
    virtual void onPreprocess() { }

private:
    /*!
     * Sets this node's parent to \a p. This function is for internal use
     * only. Use append/prepend/remove from  public API.
     *
     * It is an error to set a parent on a node which already has a parent.
     */
    void setParent(Node *p) {
        assert(m_parent == 0 || p == 0);
        m_parent = p;
    }

    Node *m_parent;
    std::vector<Node *> m_children;

    Type m_type : 4;
    unsigned m_preprocess : 1;
};



class OpacityNode : public Node {
public:
    enum { StaticType = OpacityNodeType };
    OpacityNode()
        : Node(OpacityNodeType)
        , m_opacity(1.0f)
    {
    }
    OpacityNode(float opacity)
        : Node(OpacityNodeType)
        , m_opacity(opacity)
    {
    }

    float opacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = opacity; }

private:
    float m_opacity;
};



class TransformNode : public Node
{
public:
    enum { StaticType = TransformNodeType };

    TransformNode()
        : Node(TransformNodeType)
        , m_projectionDepth(0)
    {
    }

    const mat4 &matrix() const { return m_matrix; }
    void setMatrix(const mat4 &m) { m_matrix = m; }

    float projectionDepth() const { return m_projectionDepth; }
    void setProjectionDepth(float d) { m_projectionDepth = d; }

private:
    mat4 m_matrix;
    float m_projectionDepth;
};


class RectangleNode : public Node {
public:
    enum { StaticType = RectangleNodeType };

    RectangleNode() : Node(RectangleNodeType) { }
    RectangleNode(const rect2d &geometry, const vec4 &color)
        : Node(RectangleNodeType)
        , m_geometry(geometry)
        , m_color(color)
    {
    }

    // float x() const { return m_geometry.tl.x; }
    // float y() const { return m_geometry.tl.y; }
    // vec2 position() const { return m_geometry.tl; }
    // void setPosition(float x, float y) { setPosition(vec2(x, y)); }
    // void setPosition(const vec2 &pos) { m_geometry.setPosition(pos); }

    // float width() const { return m_geometry.width(); }
    // float height() const { return m_geometry.height(); }
    // vec2 size() const { return m_geometry.size(); }
    // void setSize(float w, float h) { setSize(vec2(w, h)); }
    // void setSize(const vec2 &size) { m_geometry.setSize(size); }

    // void setGeometry(float x, float y, float w, float h) {
    //     setPosition(x, y);
    //     setSize(w, h);
    // }

    const rect2d &geometry() const { return m_geometry; }

    const vec4 &color() const { return m_color; }
    void setColor(const vec4 &color) {
        m_color = color;
        m_color.x = std::max(std::min(m_color.x, 1.0f), 0.0f);
        m_color.y = std::max(std::min(m_color.y, 1.0f), 0.0f);
        m_color.z = std::max(std::min(m_color.z, 1.0f), 0.0f);
        m_color.w = std::max(std::min(m_color.w, 1.0f), 0.0f);
    }

protected:
    RectangleNode(Type type) : Node(type) { }
    RectangleNode(Type type, const rect2d &geometry, const vec4 &color = vec4())
        : Node(type)
        , m_geometry(geometry)
        , m_color(color)
    {
    }

private:
    rect2d m_geometry;
    vec4 m_color;
};


// ### TODO: because layer node is a rectangle node it also has color. Does
// ### that really make sense?
class LayerNode : public RectangleNode {
public:
    enum { StaticType = LayerNodeType };
    LayerNode()
        : RectangleNode(LayerNodeType)
        , m_layer(0)
    {
    }

    LayerNode(const rect2d &geometry, Layer *layer)
        : RectangleNode(LayerNodeType, geometry)
        , m_layer(layer)
    {
    }

    Layer *layer() const { return m_layer; }
    void setLayer(Layer *layer) { m_layer = layer; }

private:
    Layer *m_layer;
};

RENGINE_END_NAMESPACE
