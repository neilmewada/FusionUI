// ReSharper disable CppInconsistentNaming
#pragma once

#define FNew(WidgetClass, ...) \
    (* NewObject<WidgetClass>(this))

#define FAssignNew(WidgetClass, VariableName) FNew(WidgetClass).Assign(VariableName)

#define FUSION_WIDGET(WidgetClass, SuperClass)\
	FUSION_CLASS(WidgetClass, SuperClass)

#define __FUSION_BYPASS_SETTER(PropertyType, PropertyName, DirtyFunc)\
	template<typename TSelf>\
	TSelf& __BypassSetter_##PropertyName(this TSelf& self, PropertyType const& value) {\
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
	}

#define __FUSION_PROPERTY(PropertyType, PropertyName, DirtyFunc)\
    protected:\
		PropertyType m_##PropertyName = {};\
    public:\
		PropertyType PropertyName() const { return m_##PropertyName; }\
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
		}

#define __FUSION_STYLE_PROPERTY(PropertyType, PropertyName, DirtyFunc)\
    protected:\
		PropertyType m_##PropertyName = {};\
    public:\
		PropertyType PropertyName() const { return m_##PropertyName; }\
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
		}


#define FUSION_PROPERTY(PropertyType, PropertyName) __FUSION_PROPERTY(PropertyType, PropertyName, self.MarkPaintDirty())
#define FUSION_LAYOUT_PROPERTY(PropertyType, PropertyName) __FUSION_PROPERTY(PropertyType, PropertyName, self.MarkLayoutDirty())

#define FUSION_STYLE_PROPERTY(PropertyType, PropertyName) __FUSION_STYLE_PROPERTY(PropertyType, PropertyName, self.MarkPaintDirty())

#define FUSION_PROPERTY_GET(PropertyType, PropertyName) \
	PropertyType PropertyName()

#define FUSION_PROPERTY_SET(PropertyType, PropertyName) \
	template<typename TSelf>\
	TSelf& PropertyName(this TSelf& self, PropertyType value)

#define FUSION_SIGNAL(SignalType, SignalName, ...)\
	protected:\
		SignalType m_##SignalName;\
	public:\
		auto& SignalName() { return this->m_##SignalName; }\
		template<typename TSelf, typename TLambda>\
		TSelf& SignalName(this TSelf& self, const TLambda& lambda)\
		{\
			self.m_##SignalName.Add(lambda);\
			return self;\
		}\
		template<typename TSelf, typename TLambda>\
		TSelf& SignalName(this TSelf& self, FSignalHandle& outHandle, const TLambda& lambda)\
		{\
			outHandle = self.m_##SignalName.Add(lambda);\
			return self;\
		}
