#pragma once

namespace Fusion
{

    enum class EShapeType : u32
    {
        None = 0,
        Rect,
        RoundedRect,
        Circle
    };
    FUSION_ENUM_CLASS(EShapeType);

    struct FUSIONWIDGETS_API FShape final
    {
    public:

        FShape() = default;

        FShape(EShapeType shapeType) : m_Type(shapeType)
        {
	        
        }

        static FShape RoundedRect(const FVec4& cornerRadius)
        {
            FShape shape = EShapeType::RoundedRect;
            shape.SetCornerRadius(cornerRadius);
            return shape;
        }

        EShapeType GetShapeType() const { return m_Type; }

        const FVec4& GetCornerRadius() const { return m_CornerRadius; }
        void SetCornerRadius(const FVec4& value) { m_CornerRadius = value; }

        bool operator==(const FShape& rhs) const
        {
            return m_Type == rhs.m_Type && (m_Type != EShapeType::RoundedRect || m_CornerRadius == rhs.m_CornerRadius);
        }

        bool operator!=(const FShape& rhs) const
        {
            return !operator==(rhs);
        }

    private:

        EShapeType m_Type = EShapeType::None;

        FVec4 m_CornerRadius{};

    };

    inline FShape FRectangle() { return FShape(EShapeType::Rect); }
    inline FShape FCircle() { return FShape(EShapeType::Circle); }
    inline FShape FRoundedRectangle(f32 topLeft, f32 topRight, f32 bottomRight, f32 bottomLeft)
    {
        return FShape::RoundedRect(FVec4(topLeft, topRight, bottomRight, bottomLeft));
    }
    inline FShape FRoundedRectangle(f32 cornerRadius) { return FShape::RoundedRect(FVec4(1, 1, 1, 1) * cornerRadius); }
    inline FShape FRoundedRectangle(const FVec4& cornerRadius) { return FShape::RoundedRect(cornerRadius); }
    
} // namespace Fusion
