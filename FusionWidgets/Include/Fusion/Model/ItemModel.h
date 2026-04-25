#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FItemModel;

    struct FModelIndex
    {
    public:

        FModelIndex() = default;

        FModelIndex(int row, int col, u64 internalId = 0, void* internalPtr = nullptr)
            : m_Row(row), m_Col(col), m_InternalId(internalId), m_InternalPtr(internalPtr)
        {

        }

        int   Row()         const { return m_Row; }
        int   Column()      const { return m_Col; }
        u64   InternalId()  const { return m_InternalId; }
        void* InternalPtr() const { return m_InternalPtr; }
        bool  IsValid()     const { return m_Row >= 0 && m_Col >= 0 && m_Model.IsValid(); }

        bool operator==(const FModelIndex& rhs) const
        {
            return m_Row == rhs.m_Row && m_Col == rhs.m_Col &&
                m_InternalId == rhs.m_InternalId && m_InternalPtr == rhs.m_InternalPtr &&
                    m_Model == rhs.m_Model;
        }

        bool operator!=(const FModelIndex& rhs) const
        {
            return !(*this == rhs);
        }

        SizeT GetHash() const
        {
            SizeT hash = Fusion::GetHash<int>(m_Row);
            CombineHash(hash, m_Col);
            if (m_InternalId != 0)
                CombineHash(hash, m_InternalId);
            if (m_InternalPtr != nullptr)
                CombineHash(hash, reinterpret_cast<uintptr_t>(m_InternalPtr));
            return hash;
        }

    private:
        int m_Row = -1;
        int m_Col = 0;

        u64   m_InternalId = 0;
        void* m_InternalPtr = nullptr;

        WeakRef<FItemModel> m_Model;

        friend class FItemModel;
    };

    class FUSIONWIDGETS_API FItemModel : public FObject
    {
    public:

        FModelIndex CreateIndex(u32 row, u32 column, u64 internalId = 0, void* internalPtr = nullptr)
        {
            FModelIndex index(row, column, internalId, internalPtr);
            index.m_Model = Ref(this);
            return index;
        }

        virtual FModelIndex GetParent(const FModelIndex& index) = 0;

        virtual FModelIndex GetIndex(u32 row, u32 column, const FModelIndex& parent = {}) = 0;

        virtual u32 GetRowCount(const FModelIndex& parent = {}) = 0;
        virtual u32 GetColumnCount(const FModelIndex& parent = {}) = 0;

    private:

    };
    
} // namespace Fusion
