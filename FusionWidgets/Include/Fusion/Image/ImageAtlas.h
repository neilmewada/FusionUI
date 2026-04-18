#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FImageAtlas : public FObject
    {
        FUSION_CLASS(FImageAtlas, FObject)
    protected:

        FImageAtlas(Ref<FApplicationInstance> application);

    public:

        static constexpr u32 kAtlasSize = 4096;
        static constexpr u32 kMaxLayers = 8;
        static constexpr u32 kIconPadding = 4;

        ~FImageAtlas();

        void Initialize();

        void Shutdown();

        FAtlasHandle GetHandle() const { return m_AtlasHandle; }

        struct FAtlasItem
        {
            int layerIndex = -1;
            FVec2 uvMin, uvMax;
            u32 width = 0, height = 0;

            bool IsValid() const { return layerIndex >= 0; }
        };

        FAtlasItem AddImage(const FName& name, const FImage& image);
        bool RemoveImage(const FName& name);

        FAtlasItem FindImage(const FName& name);

        struct FUSIONCORE_API FBinaryNode : FIntrusiveBase
        {
            FStaticArray<IPtr<FBinaryNode>, 2> Child;
            FBinaryNode* Parent = nullptr;
            int TotalChildren = 0;
            u32 UsedArea = 0;

            FRect Rect;
            FName ImageName;
            // For debug use only!
            int ImageId = -1;

            u32 GetTotalArea() const
            {
                return FMath::RoundToInt(Rect.GetSize().width * Rect.GetSize().height);
            }

            u32 GetFreeArea() const
            {
                return GetTotalArea() - UsedArea;
            }

            bool IsLeaf() const
            {
                return Child[0] == nullptr && Child[1] == nullptr;
            }

            bool IsWidthSpan() const
            {
                return !IsLeaf() && Child[0]->Rect.max.x < Child[1]->Rect.max.x;
            }

            bool IsHeightSpan() const
            {
                return !IsLeaf() && Child[0]->Rect.max.y < Child[1]->Rect.max.y;
            }

            bool IsValid() const
            {
                return ImageName.IsValid();
            }

            bool IsValidRecursive() const
            {
                return IsValid() || (Child[0] != nullptr && Child[0]->IsValidRecursive()) ||
                    (Child[1] != nullptr && Child[1]->IsValidRecursive());
            }

            FVec2i GetSize() const
            {
                return FVec2i(FMath::RoundToInt(Rect.GetSize().width), FMath::RoundToInt(Rect.GetSize().height));
            }

            void ClearImage();

            template<typename TFunc>
            void ForEachRecursive(const TFunc& visitor)
            {
                visitor(this);

                if (Child[0] != nullptr)
                {
                    Child[0]->ForEachRecursive(visitor);
                }
                if (Child[1] != nullptr)
                {
                    Child[1]->ForEachRecursive(visitor);
                }
            }

            IPtr<FBinaryNode> FindUsedNode();

            IPtr<FBinaryNode> Insert(FVec2i imageSize);

            bool DefragmentFast();
            bool DefragmentSlow();
        };

    private:

        // - Atlas -

        struct FAtlasLayer : FIntrusiveBase
        {
            FAtlasLayer()
            {
                LayerIndex = 0;
                Root = new FBinaryNode;

                Root->Rect = FRect(0, 0, kAtlasSize, kAtlasSize);
            }

            virtual ~FAtlasLayer()
            {
                Root = nullptr;
            }

            int LayerIndex = 0;

            IPtr<FBinaryNode> Root = nullptr;
            FHashMap<FName, IPtr<FBinaryNode>> NodesByImageName;
        };

        FArray<IPtr<FAtlasLayer>> m_Layers;

        WeakRef<FApplicationInstance> m_ApplicationInstance;

        FAtlasHandle m_AtlasHandle;

        // - Cache -

        FHashMap<FName, FAtlasItem> m_ImagesByName;
        FAtlasItem m_WhitePixel{};
        FAtlasItem m_TransparentPixel{};
    };
    
} // namespace Fusion
