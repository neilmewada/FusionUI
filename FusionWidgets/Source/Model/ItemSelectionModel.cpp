
#include "Fusion/Widgets.h"

namespace Fusion
{
    FItemSelectionModel::FItemSelectionModel()
    {

    }

    void FItemSelectionModel::ClearSelection()
    {
        FModelIndexList removed = MoveTemp(m_SelectedIndices);
        m_SelectedIndices.Clear();
        m_OnSelectionChanged.Broadcast({}, removed);
    }

    void FItemSelectionModel::Select(FModelIndex index, ESelectionFlags selectionFlags = Select_Current)
    {
        thread_local FModelIndexList removed{};
        thread_local FModelIndexList added{};
        removed.Clear();
        added.Clear();

        if (selectionFlags & Select_Clear)
        {
            removed = MoveTemp(m_SelectedIndices);
            m_SelectedIndices.Clear();
        }

        if (selectionFlags & Select_Current)
        {
            if (index.IsValid())
            {
                added.Add(index);
                m_SelectedIndices.Add(index);
                m_CurrentIndex = index;
            }
            else
            {
                m_CurrentIndex = {};
            }
        }

        if (index.IsValid() && (selectionFlags & Select_Toggle))
        {
            if (IsSelected(index))
            {
                removed.Add(index);
                m_SelectedIndices.Remove(index);
            }
            else
            {
                added.Add(index);
                m_SelectedIndices.Add(index);
            }
        }

        if (!added.Empty() || !removed.Empty())
        {
            m_OnSelectionChanged.Broadcast(added, removed);
        }
    }

    bool FItemSelectionModel::IsSelected(FModelIndex index)
    {
        return m_SelectedIndices.Contains(index);
    }

}
