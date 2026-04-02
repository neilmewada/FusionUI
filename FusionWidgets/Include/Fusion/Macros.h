// ReSharper disable CppInconsistentNaming
#pragma once

#define FNew(WidgetClass, ...) \
    (* NewObject<WidgetClass>(__VA_ARGS__))

#define FUSION_WIDGET


#define __FUSION_PROPERTY(PropertyType, PropertyName, DirtyFunc)\
    protected:\
		PropertyType m_##PropertyName = {};\
    public:\
		template<typename TSelf>\
		TSelf& PropertyName(this TSelf& self, PropertyType const& value) {\
			ZoneScoped;\
			if constexpr (TFEquitable<PropertyType>::Value)\
			{\
				if (TFEquitable<PropertyType>::AreEqual(self.m_##PropertyName, value))\
					return self;\
			}\
			thread_local const Fusion::FName nameValue = #PropertyName;\
			self.m_##PropertyName = value;\
			if ((self.GetFlags() & EObjectFlags::PendingConstruction) == 0) {\
				 static_cast<Fusion::FWidget&>(self).OnPropertyModified(nameValue);\
				 DirtyFunc;\
			}\
			return self;\
		}\

#define FUSION_PROPERTY(PropertyType, PropertyName) __FUSION_PROPERTY(PropertyType, PropertyName, self.MarkPaintDirty())
#define FUSION_LAYOUT_PROPERTY(PropertyType, PropertyName) __FUSION_PROPERTY(PropertyType, PropertyName, self.MarkLayoutDirty())


