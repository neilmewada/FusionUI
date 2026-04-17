// ReSharper disable CppInconsistentNaming
#pragma once

#include <optional>

#define FNew(WidgetClass, ...) \
    (* ::Fusion::NewObject<WidgetClass>(this, ##__VA_ARGS__))

#define FAssignNew(WidgetClass, VariableName) FNew(WidgetClass).Assign(VariableName)

#define FUSION_WIDGET(WidgetClass, SuperClass)\
	FUSION_CLASS(WidgetClass, SuperClass)

#define __FUSION_STYLE_BYPASS_SETTER(PropertyType, PropertyName, DirtyFunc)\
	void __StyleBypassSetter_##PropertyName(PropertyType const& value) {\
		ZoneScoped; auto& self = *this;\
		if constexpr (TFEquitable<PropertyType>::Value)\
		{\
			if (TFEquitable<PropertyType>::AreEqual(m_##PropertyName, value))\
				return;\
		}\
		thread_local const Fusion::FName nameValue = #PropertyName;\
		m_##PropertyName = value;\
		if ((GetFlags() & EObjectFlags::PendingConstruction) == 0) {\
			 OnPropertyModified(nameValue);\
			 DirtyFunc;\
		}\
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
		}\
		__FUSION_STYLE_BYPASS_SETTER(PropertyType, PropertyName, DirtyFunc)\
		void __AnimBypassSetter_##PropertyName(PropertyType const& value) {\
			ZoneScoped; auto& self = *this;\
			if constexpr (TFEquitable<PropertyType>::Value)\
			{\
				if (TFEquitable<PropertyType>::AreEqual(m_##PropertyName, value))\
					return;\
			}\
			thread_local const Fusion::FName nameValue = #PropertyName;\
			m_##PropertyName = value;\
			if ((GetFlags() & EObjectFlags::PendingConstruction) == 0) {\
				 OnPropertyModified(nameValue);\
				 DirtyFunc;\
			}\
		}

#define __FUSION_STYLE_PROPERTY(PropertyType, PropertyName, DirtyFunc)\
    protected:\
		std::optional<PropertyType> m_Inline##PropertyName;\
		PropertyType m_##PropertyName = {};\
    public:\
		PropertyType PropertyName() const { return m_Inline##PropertyName.has_value() ? m_Inline##PropertyName.value() : m_##PropertyName; }\
		template<typename TSelf>\
		TSelf& PropertyName(this TSelf& self, PropertyType const& value) {\
			ZoneScoped;\
			if constexpr (TFEquitable<PropertyType>::Value)\
			{\
				if (self.m_Inline##PropertyName.has_value() && TFEquitable<PropertyType>::AreEqual(self.m_Inline##PropertyName.value(), value))\
					return self;\
			}\
			thread_local const Fusion::FName nameValue = #PropertyName;\
			self.m_Inline##PropertyName = value;\
			self.m_##PropertyName = value;\
			if ((self.GetFlags() & EObjectFlags::PendingConstruction) == 0) {\
				 static_cast<Fusion::FWidget&>(self).OnPropertyModified(nameValue);\
				 DirtyFunc;\
			}\
			return self;\
		}\
		__FUSION_STYLE_BYPASS_SETTER(PropertyType, PropertyName, DirtyFunc)\
		void __AnimBypassSetter_##PropertyName(PropertyType const& value) {\
			ZoneScoped; auto& self = *this;\
			if constexpr (TFEquitable<PropertyType>::Value)\
			{\
				if (self.m_Inline##PropertyName.has_value() && TFEquitable<PropertyType>::AreEqual(self.m_Inline##PropertyName.value(), value))\
					return;\
				if (!self.m_Inline##PropertyName.has_value() && TFEquitable<PropertyType>::AreEqual(self.m_##PropertyName, value))\
					return;\
			}\
			thread_local const Fusion::FName nameValue = #PropertyName;\
			if (self.m_Inline##PropertyName.has_value()) m_Inline##PropertyName = value;\
			else self.m_##PropertyName = value;\
			if ((GetFlags() & EObjectFlags::PendingConstruction) == 0) {\
				 OnPropertyModified(nameValue);\
				 DirtyFunc;\
			}\
		}


#define FUSION_PROPERTY(PropertyType, PropertyName) __FUSION_PROPERTY(PropertyType, PropertyName, self.MarkPaintDirty())
#define FUSION_LAYOUT_PROPERTY(PropertyType, PropertyName) __FUSION_PROPERTY(PropertyType, PropertyName, self.MarkLayoutDirty())

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

#define __FUSION_APPLY_STYLE(PropertyName)\
	if (!m_Inline##PropertyName.has_value())\
	{\
		decltype(m_##PropertyName) value;\
		if (style.TryGet(#PropertyName, value, GetStyleState()))\
		{\
			bool areEqual = false;\
			if constexpr (TFEquitable<decltype(m_##PropertyName)>::Value)\
			{\
				areEqual = TFEquitable<decltype(m_##PropertyName)>::AreEqual(m_##PropertyName, value);\
			}\
			if constexpr (FAnimatable<decltype(m_##PropertyName)>::Supported)\
			{\
				FTransition transition;\
				if (!areEqual && style.TryGetTransition(#PropertyName, transition))\
				{\
					if (transition.Type == ETransitionType::Tween)\
					{\
						__FAnimate_Tween(this, PropertyName, __StyleBypassSetter_)\
						.To(value)\
						.Duration(transition.Tween.Duration)\
						.Easing(transition.Tween.Easing)\
						.Delay(transition.Tween.Delay)\
						.Play();\
					}\
					else if (transition.Type == ETransitionType::Spring)\
					{\
						__FAnimate_Spring(this, PropertyName, __StyleBypassSetter_)\
						.Target(value)\
						.Damping(transition.Spring.Damping)\
						.Stiffness(transition.Spring.Stiffness)\
						.Delay(transition.Spring.Delay)\
						.Play();\
					}\
					else\
					{\
						__StyleBypassSetter_##PropertyName(value);\
					}\
				}\
				else if (!areEqual)\
				{\
					__StyleBypassSetter_##PropertyName(value);\
				}\
			}\
			else if (!areEqual)\
			{\
				__StyleBypassSetter_##PropertyName(value);\
			}\
		}\
	}

#define FUSION_APPLY_STYLES(...) FUSION_FOR_EACH(FUSION_APPLY_STYLE, __VA_ARGS__)

// Per-tuple helpers used by FUSION_STYLE_PROPERTIES.
// Each tuple is a parenthesised (Type, Name, DirtyKind) token group
// where DirtyKind is Paint or Layout.

// Dirty-kind dispatch — maps the trailing tag to the right property macro.
#define __FUSION_SP_PROP_Paint(Type, Name)   __FUSION_STYLE_PROPERTY(Type, Name, self.MarkPaintDirty())
#define __FUSION_SP_PROP_Layout(Type, Name)  __FUSION_STYLE_PROPERTY(Type, Name, self.MarkLayoutDirty())
#define __FUSION_SP_PROP_LayoutAndPaint(Type, Name)  __FUSION_STYLE_PROPERTY(Type, Name, self.MarkLayoutDirty(); self.MarkPaintDirty())
#define __FUSION_SP_PROP_PaintAndLayout(Type, Name)  __FUSION_STYLE_PROPERTY(Type, Name, self.MarkLayoutDirty(); self.MarkPaintDirty())

// DECL: unpack via juxtaposition, then dispatch on the trailing DirtyKind tag.
#define __FUSION_SP_DECL(Tuple)                    __FUSION_SP_DECL_I Tuple
#define __FUSION_SP_DECL_I(Type, Name, DirtyKind)  FUSION_CONCATENATE(__FUSION_SP_PROP_, DirtyKind)(Type, Name)

// APPLY: extract Name (2nd element) with an extra indirection so it is fully
//        expanded before reaching the # / ## operators inside __FUSION_APPLY_STYLE.
#define __FUSION_SP_NAME(Type, Name, DirtyKind)  Name
#define __FUSION_SP_APPLY(Tuple)                 __FUSION_SP_APPLY2(__FUSION_SP_NAME Tuple)
#define __FUSION_SP_APPLY2(Name)                 __FUSION_APPLY_STYLE(Name)

// For internal use only! Do not use this!
#define __FUSION_STYLE_PROPERTIES_FWIDGET(...) \
    FUSION_MACRO_EXPAND(FUSION_FOR_EACH(__FUSION_SP_DECL, __VA_ARGS__)) \
    virtual void ApplyStyle(FStyle& style) \
    { \
        FUSION_MACRO_EXPAND(FUSION_FOR_EACH(__FUSION_SP_APPLY, __VA_ARGS__)) \
    }

// Declares every (Type, Name, DirtyKind) style property AND generates the ApplyStyle override.
// Replaces individual FUSION_STYLE_PROPERTY calls + the hand-written ApplyStyle body.
#define FUSION_STYLE_PROPERTIES(...) \
    FUSION_MACRO_EXPAND(FUSION_FOR_EACH(__FUSION_SP_DECL, __VA_ARGS__)) \
    void ApplyStyle(FStyle& style) override \
    { \
        Super::ApplyStyle(style); \
        FUSION_MACRO_EXPAND(FUSION_FOR_EACH(__FUSION_SP_APPLY, __VA_ARGS__)) \
    }
