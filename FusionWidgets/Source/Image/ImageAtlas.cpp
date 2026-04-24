#include "Fusion/Widgets.h"
#include "Fusion/Resources.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    FImageAtlas::FImageAtlas(Ref<FApplicationInstance> application) : Super("ImageAtlas"), m_ApplicationInstance(application)
    {
        IFRenderBackend* renderBackend = application->GetRenderBackend();

        m_AtlasHandle = renderBackend->CreateLayeredAtlas(false, kAtlasSize, 1);
    }

    FImageAtlas::~FImageAtlas()
    {
        Shutdown();
    }

    void FImageAtlas::Initialize()
    {
        ZoneScoped;

        if (!m_Layers.Empty())
            return;

        IPtr<FAtlasLayer> layer = new FAtlasLayer;
        layer->LayerIndex = 0;

        m_Layers.Add(layer);

        {
            u32 pixels[16];
            for (int i = 0; i < FUSION_COUNT(pixels); ++i)
            {
                pixels[i] = TNumericLimits<u32>::Max();
            }
            IPtr<FImage> image = FImage::CreateExternal("__WhitePixel", EImageFormat::RGBA8, 4, 4, (u8*)pixels);
            m_WhitePixel = AddImage("__WhitePixel", *image);
        }

        {
            u32 pixels[16];
            for (int i = 0; i < FUSION_COUNT(pixels); ++i)
            {
                pixels[i] = 0;
            }
            IPtr<FImage> image = FImage::CreateExternal("__TransparentPixel", EImageFormat::RGBA8, 4, 4, (u8*)pixels);
            m_TransparentPixel = AddImage("__TransparentPixel", *image);
        }

        for (Resources::FResource resource : Resources::ListAll("embed:/Icons"))
        {
            IPtr<FImage> image = FImage::CreateFromMemory("temp", resource.Data, resource.Size, 4);
            AddImage(resource.Path, *image);
        }
    }

    void FImageAtlas::Shutdown()
    {
        ZoneScoped;

        if (Ref<FApplicationInstance> applicationInstance = m_ApplicationInstance.Lock())
        {
            applicationInstance->GetRenderBackend()->DestroyAtlas(m_AtlasHandle);
            m_AtlasHandle = {};
        }
    }

    FImageAtlas::FAtlasItem FImageAtlas::AddImage(const FName& name, const FImage& imageSource)
    {
        if (!name.IsValid() || !imageSource.IsValid() || m_ImagesByName.KeyExists(name))
            return {};

        Ref<FApplicationInstance> application = m_ApplicationInstance.Lock();
        if (!application)
            return {};

        FVec2i textureSize = FVec2i(imageSource.Width + kIconPadding, imageSource.Height + kIconPadding);
        u32 textureArea = textureSize.width * textureSize.height;

        IPtr<FAtlasLayer> foundAtlas = nullptr;
        IPtr<FBinaryNode> insertNode = nullptr;

        for (int i = 0; i < m_Layers.Size(); ++i)
        {
            IPtr<FAtlasLayer> atlas = m_Layers[i];

            insertNode = atlas->Root->Insert(textureSize);

            if (insertNode == nullptr && atlas->Root->GetFreeArea() > textureArea * 8)
            {
                atlas->Root->DefragmentSlow();

                insertNode = atlas->Root->Insert(textureSize);
            }

            if (insertNode != nullptr)
            {
                foundAtlas = atlas;
                break;
            }
        }

	    // TODO: Add a new atlas layer if insertNode is nullptr here.
        FUSION_ASSERT(insertNode, "TODO: Add a new atlas layer");

        textureSize -= FVec2i(kIconPadding, kIconPadding);

        insertNode->ImageName = name;
        
        foundAtlas->Root->UsedArea += textureArea;
        foundAtlas->NodesByImageName[name] = insertNode;

        int posX = FMath::RoundToInt(insertNode->Rect.min.x) + kIconPadding / 2;
        int posY = FMath::RoundToInt(insertNode->Rect.min.y) + kIconPadding / 2;

        FVec2 uvMin = FVec2(((f32)posX + 0.5f) / (f32)kAtlasSize, ((f32)posY + 0.5f) / (f32)kAtlasSize);
        FVec2 uvMax = FVec2((f32)(posX + textureSize.width - 0.5f) / (f32)kAtlasSize, (f32)(posY + textureSize.height - 0.5f) / (f32)kAtlasSize);

        IFRenderBackend* backend = application->GetRenderBackend();

        backend->UploadAtlasRegionAsync(m_AtlasHandle, foundAtlas->LayerIndex, FVec2i(posX, posY), textureSize, imageSource.Data(), imageSource.Pitch());

        FAtlasItem item = { .layerIndex = foundAtlas->LayerIndex, .uvMin = uvMin, .uvMax = uvMax };
        item.width = textureSize.width;
        item.height = textureSize.height;

        m_ImagesByName[name] = item;

        return item;
    }

    bool FImageAtlas::RemoveImage(const FName& name)
    {
        if (!m_ImagesByName.KeyExists(name))
            return false;

        int layerIndex = m_ImagesByName[name].layerIndex;
        if (layerIndex < 0 || layerIndex >= m_Layers.Size())
            return false;

        IPtr<FAtlasLayer> atlas = m_Layers[layerIndex];

        if (!atlas->NodesByImageName.KeyExists(name))
            return false;

        IPtr<FBinaryNode> node = atlas->NodesByImageName[name];
        if (node == nullptr)
            return false;

        atlas->Root->UsedArea -= node->Rect.GetAreaInt();
        node->ClearImage();
        atlas->Root->DefragmentFast();

        atlas->NodesByImageName.Remove(name);
        m_ImagesByName.Remove(name);

        return true;
    }

    FImageAtlas::FAtlasItem FImageAtlas::FindImage(const FName& name)
    {
        auto it = m_ImagesByName.Find(name);
        if (it == m_ImagesByName.End())
            return {};

        return it->second;
    }

    // -------------------------------------------------------------------------------
    // - Binary Node -

    void FImageAtlas::FBinaryNode::ClearImage()
    {
        ImageName = FName();
        ImageId = -1;
    }

    IPtr<FImageAtlas::FBinaryNode> FImageAtlas::FBinaryNode::FindUsedNode()
    {
        if (IsValid())
            return this;

        if (Child[0] != nullptr)
        {
            IPtr<FBinaryNode> node = Child[0]->FindUsedNode();
            if (node != nullptr)
            {
                return node;
            }
        }

        if (Child[1] != nullptr)
        {
            IPtr<FBinaryNode> node = Child[1]->FindUsedNode();
            if (node != nullptr)
            {
                return node;
            }
        }

        return nullptr;
    }

    IPtr<FImageAtlas::FBinaryNode> FImageAtlas::FBinaryNode::Insert(FVec2i imageSize)
    {
        if (Child[0] != nullptr)
        {
            // We are not in leaf node
            IPtr<FBinaryNode> newNode = Child[0]->Insert(imageSize);
            if (newNode != nullptr)
            {
                TotalChildren++;
                return newNode;
            }
            if (Child[1] == nullptr)
            {
                return nullptr;
            }

            newNode = Child[1]->Insert(imageSize);
            if (newNode != nullptr)
            {
                TotalChildren++;
            }

            return newNode;
        }
        else // We are in leaf node
        {
            if (IsValid()) // Do not split a valid node
                return nullptr;

            if (GetSize().width < imageSize.width ||
                GetSize().height < imageSize.height)
            {
                return nullptr;
            }

            if (GetSize().width == imageSize.width && GetSize().height == imageSize.height)
            {
                return this;
            }

            // Split the node
            Child[0] = new FBinaryNode;
            Child[1] = new FBinaryNode;
            Child[0]->Parent = this;
            Child[1]->Parent = this;

            TotalChildren = 2;

            int dw = GetSize().width - imageSize.width;
            int dh = GetSize().height - imageSize.height;

            if (dw > dh)
            {
                Child[0]->Rect = FRect(Rect.left, Rect.top,
                    Rect.left + imageSize.width, Rect.bottom);
                Child[1]->Rect = FRect(Rect.left + imageSize.width + 1, Rect.top,
                    Rect.right, Rect.bottom);
            }
            else
            {
                Child[0]->Rect = FRect(Rect.left, Rect.top,
                    Rect.right, Rect.top + imageSize.height);
                Child[1]->Rect = FRect(Rect.left, Rect.top + imageSize.height + 1,
                    Rect.right, Rect.bottom);
            }

            return Child[0]->Insert(imageSize);
        }
    }

    bool FImageAtlas::FBinaryNode::DefragmentFast()
    {
        if (Child[0] != nullptr && Child[1] != nullptr)
        {
            ZoneScoped;

            bool leftValid = Child[0]->DefragmentFast();
            bool rightValid = Child[1]->DefragmentFast();

            if (!leftValid && !rightValid)
            {
                Child[0] = nullptr;
                Child[1] = nullptr;

                return false;
            }

            // Perform basic defragmentation:
            // Merge two empty consecutive columns OR rows into a single one!
            // This can happen if the 2nd consecutive column/row is the 1st child of 2nd node.
            {
                if (IsWidthSpan() && !leftValid && Child[1]->Child[0] != nullptr &&
                    Child[1]->IsWidthSpan() && !Child[1]->Child[0]->IsValidRecursive())
                {
                    IPtr<FBinaryNode> nodeToMove = Child[1]->Child[1];
                    Child[0]->Rect.max.x = Child[1]->Child[0]->Rect.max.x;
                    Child[1] = nodeToMove;
                    nodeToMove->Parent = this;
                }
                else if (IsHeightSpan() && !leftValid && Child[1]->Child[0] != nullptr &&
                    Child[1]->IsHeightSpan() && !Child[1]->Child[0]->IsValidRecursive())
                {
                    IPtr<FBinaryNode> nodeToMove = Child[1]->Child[1];
                    Child[0]->Rect.max.y = Child[1]->Child[0]->Rect.max.y;
                    Child[1] = nodeToMove;
                    nodeToMove->Parent = this;
                }
            }

            return true;
        }

        return IsValid();
    }

    bool FImageAtlas::FBinaryNode::DefragmentSlow()
    {
        if (Child[0] != nullptr && Child[1] != nullptr)
        {
            ZoneScoped;

            bool leftValid = Child[0]->DefragmentSlow();
            bool rightValid = Child[1]->DefragmentSlow();

            if (!leftValid && !rightValid)
            {
                return false;
            }

            // Perform advanced defragmentation which is very slow:
            {
                if (IsWidthSpan() && !leftValid && Child[1]->Child[0] != nullptr &&
                    Child[1]->IsHeightSpan() && !Child[1]->Child[0]->IsValidRecursive() &&
                    Child[1]->Child[1] != nullptr && Child[1]->Child[1]->IsWidthSpan() &&
                    Child[1]->Child[1]->Child[0] != nullptr && !Child[1]->Child[1]->Child[0]->IsValidRecursive())
                {
                    IPtr<FBinaryNode> nodeToMove = Child[1]->Child[1]->Child[1];
                    f32 splitX = Child[1]->Child[1]->Child[0]->Rect.max.x;

                    Child[0]->Rect.max.x = splitX;
                    Child[1]->Rect.min.x = splitX + 1;
                    Child[1]->Child[0]->Rect.min.x = splitX + 1;
                    Child[1]->Child[1] = nodeToMove;
                    nodeToMove->Parent = Child[1]->Child[1];

                    if (nodeToMove->IsHeightSpan() && !nodeToMove->Child[0]->IsValidRecursive())
                    {
                        IPtr<FBinaryNode> contentNode = nodeToMove->Child[1];
                        Child[1]->Child[0]->Rect.max.y = contentNode->Rect.min.y - 1;
                        Child[1]->Child[1] = contentNode;
                        contentNode->Parent = Child[1]->Child[1];
                    }
                }
            }

            return true;
        }

        return IsValid();
    }

} // namespace Fusion
