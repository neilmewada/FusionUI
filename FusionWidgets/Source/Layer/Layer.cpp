#pragma once

namespace Fusion
{
	FLayer::FLayer(FName name, FObject* outer)
	{
		
	}

	f32 FLayer::GetDpiScale()
	{
		if (Ptr<FWidget> widget = m_OwningWidget.Lock())
		{
			if (Ptr<FSurface> surface = widget->GetParentSurface())
			{
				return surface->GetDpiScale();
			}
		}
		
		return 1.0f;
	}

	bool FLayer::NeedsRepaint()
	{
		if (Ptr<FWidget> widget = m_OwningWidget.Lock())
		{
			return widget->IsPaintDirty();
		}
		return false;
	}

	void FLayer::DoPaintIfNeeded()
	{
		if (NeedsRepaint())
		{
			DoPaint();
		}
		else
		{
			for (auto layer : children)
			{
				layer->DoPaintIfNeeded();
			}
		}
	}

	FAffineTransform FLayer::GetGlobalTransform()
	{
		FAffineTransform global = GetTransformInParentSpace();

		Ptr<FLayer> parent = this->parent.Lock();

		while (parent != nullptr)
		{
			global = parent->GetTransformInParentSpace() * global;

			parent = parent->parent.Lock();
		}

		return global;
	}

	void FLayer::DoPaint()
	{
	}

	void FLayer::DoPaint(FWidget* widget, FPainter& painter)
	{

	}
} // namespace Fusion
