
#include <Fusion/Core.h>
#include <Fusion/SDL3Platform.h>
#include <Fusion/Widgets.h>

using namespace Fusion;

class SampleWindow : public FDecoratedWidget
{
	FUSION_WIDGET(SampleWindow, FDecoratedWidget)
public:

	Ref<FVerticalStack> vstack;
	Ref<FHorizontalStack> hstack;

	Ref<FButton> btn0;

	void Construct() override
	{
		Super::Construct();

		Background(FColor(0.13f, 0.13f, 0.15f));

		FGradient gradient =
			FGradient::Linear(45.0f)
			.AddStop(FColor(0.06f, 0.01f, 0.18f), 0.0f)   // deep violet
			.AddStop(FColor(0.49f, 0.07f, 0.64f), 0.35f)  // vivid purple
			.AddStop(FColor(0.93f, 0.26f, 0.42f), 0.65f)  // hot pink
			.AddStop(FColor(1.00f, 0.72f, 0.20f), 1.0f);  // warm gold

		FPen gradientPen = FPen::Gradient(
			FGradient::Linear()
			.AddStop(FColor(1.00f, 0.00f, 0.50f), 0.000f) // magenta
			.AddStop(FColor(1.00f, 0.00f, 0.00f), 0.125f) // red
			.AddStop(FColor(1.00f, 0.50f, 0.00f), 0.250f) // orange
			.AddStop(FColor(1.00f, 1.00f, 0.00f), 0.375f) // yellow
			.AddStop(FColor(0.00f, 1.00f, 0.00f), 0.500f) // green
			.AddStop(FColor(0.00f, 1.00f, 1.00f), 0.625f) // cyan
			.AddStop(FColor(0.00f, 0.00f, 1.00f), 0.750f) // blue
			.AddStop(FColor(0.50f, 0.00f, 1.00f), 0.875f) // violet
			.AddStop(FColor(1.00f, 0.00f, 0.50f), 1.000f) // magenta
		)
		.Thickness(3.0f)
		.GradientSpace(EGradientSpace::ArcLength);


		Child(
			FAssignNew(FVerticalStack, vstack)
			.ContentHAlign(EHAlign::Fill)
			.HAlign(EHAlign::Fill)
			.VAlign(EVAlign::Fill)
			.Padding(FMargin(1, 1, 1, 1) * 5)
			.Spacing(10)
			.Name("RootStack")
			(
				FNew(FWidget)
				.Height(25)
				.Name("Empty"),

				FAssignNew(FHorizontalStack, hstack)
				.ContentHAlign(EHAlign::Center)
				.ContentVAlign(EVAlign::Center)
				//.ForcePaintBoundary(true)
				.Spacing(10)
				.ClipContent(true)
				.Name("hstack")
				(
					FAssignNew(FButton, btn0)
					.FillRatio(1.0f)
					.Height(32)
					.Style("Button/Primary")
					.OnClick([]
					{
						FUSION_LOG_INFO("Debug", "Primary clicked!");
					}),

					FNew(FButton)
					.FillRatio(1.0f)
					.Height(32)
					.Style("Button/Secondary")
					.OnClick([]
					{
						FUSION_LOG_INFO("Debug", "Secondary clicked!");
					}),

					FNew(FButton)
					.FillRatio(1.0f)
					.Height(32)
					.Style("Button/Destructive")
					.OnClick([this]
					{
						btn0->Disabled(!btn0->Disabled());
						FUSION_LOG_INFO("Debug", "Destructive clicked!");
					}),

					FNew(FButton)
					.FillRatio(1.0f)
					.Height(32)
					.Style("Button/Primary")
					.Background(FColors::Gold)
					.Border(FPen())
				),

				FNew(FDecoratedWidget)
				.Border(gradientPen)
				.Background(FColors::White)
				.Shape(FRoundedRectangle(5.0f))
				.Height(35),

				FNew(FDecoratedWidget)
				.Background(gradient)
				.Height(100),

				FNew(FWidget)
				.FillRatio(1.0f)
			)
		);
	}

	SampleWindow()
	{
		m_GradientOffset = 0.0f;
		m_DashLength = 10.0f;
		m_DashGap = 5.0f;
		m_DashPhase = 0.0f;
	}
	
	void PaintOverlay(FPainter& painter) override
	{
		FPen gradientPen = FPen::Gradient(
			FGradient::Linear()
			.AddStop(FColor(0.10f, 0.20f, 1.00f), 0.00f) // blue
			.AddStop(FColor(0.00f, 0.85f, 0.90f), 0.17f) // cyan
			.AddStop(FColor(0.10f, 0.90f, 0.30f), 0.33f) // green
			.AddStop(FColor(0.95f, 0.95f, 0.10f), 0.50f) // yellow
			.AddStop(FColor(0.10f, 0.90f, 0.30f), 0.67f) // green
			.AddStop(FColor(0.00f, 0.85f, 0.90f), 0.83f) // cyan
			.AddStop(FColor(0.10f, 0.20f, 1.00f), 1.00f) // blue
		)
		.DashGap(m_DashGap)
		.DashLength(m_DashLength)
		.DashGap(m_DashGap)
		.DashPhase(m_DashPhase)
		.Style(EPenStyle::Dashed)
		.Thickness(3.0f)
		.GradientSpace(EGradientSpace::WorldSpace);

		painter.SetPen(gradientPen);

		const FVec2 sz = GetLayoutSize();

		// Drawing area: horizontally centered, in the lower half
		const f32 drawW = FMath::Min(sz.x * 0.80f, 500.0f);
		const f32 drawH = drawW * 0.35f;
		const FVec2 orig = FVec2((sz.x - drawW) * 0.5f, sz.y * 0.62f);

		auto P = [&](f32 x, f32 y) -> FVec2
		{
			return FVec2(orig.x + x * drawW, orig.y + y * drawH);
		};
		
		// Segment 1: straight horizontal line from left
		painter.PathInsert(P(0.00f, 0.50f));
		painter.PathInsert(P(0.20f, 0.50f));

		// Segment 2: cubic curve swooping up then down
		painter.PathBezierCubicCurveTo(
			P(0.20f, 0.50f), P(0.30f, 0.00f), P(0.45f, 0.00f), P(0.50f, 0.50f));

		// Segment 3: straight line through the middle
		painter.PathInsert(P(0.65f, 0.50f));

		// Segment 4: cubic curve swooping down then up to the right end
		painter.PathBezierCubicCurveTo(
			P(0.65f, 0.50f), P(0.75f, 1.00f), P(0.88f, 1.00f), P(1.00f, 0.50f));

		painter.PathStroke(false);
	}

	FUSION_PROPERTY(f32, GradientOffset);
	FUSION_PROPERTY(f32, DashLength);
	FUSION_PROPERTY(f32, DashGap);
	FUSION_PROPERTY(f32, DashPhase);
};

int main(int argc, char* argv[])
{
	FApplication app(argc, argv);

	Ref<FTheme> theme = app.CreateDefaultTheme();

	theme->MergeStyleSheet(FUSION_STYLE_SHEET
	{
		FUSION_STYLE(FButton, "Button/Base", Shape, Background, Border)
		{
			Shape = FRoundedRectangle(5.0f);

			FUSION_ON(Disabled)
			{
				Background = FColor(0.18f, 0.18f, 0.20f);
				Border     = FColor(0.25f, 0.25f, 0.28f);
			}
		}

		FUSION_STYLE(FButton, "Button/Primary", Shape, Background, Border)
		{
			Extends("Button/Base");
			Background = FColor(0.23f, 0.51f, 0.96f);
			Border     = FColor(0.16f, 0.40f, 0.82f);

			FUSION_ON(Hovered)
			{
				Background = FColor(0.38f, 0.65f, 0.98f);
				Border     = FColor(0.26f, 0.53f, 0.90f);
			}
			FUSION_ON(Pressed, Hovered)
			{
				Background = FColor(0.11f, 0.31f, 0.85f);
				Border     = FColor(0.08f, 0.23f, 0.70f);
			}
		}

		FUSION_STYLE(FButton, "Button/Secondary", Shape, Background, Border)
		{
			Extends("Button/Base");
			Background = FColor(0.22f, 0.22f, 0.25f);
			Border     = FColor(0.38f, 0.38f, 0.43f);

			FUSION_ON(Hovered)
			{
				Background = FColor(0.29f, 0.29f, 0.33f);
				Border     = FColor(0.50f, 0.50f, 0.56f);
			}
			FUSION_ON(Pressed, Hovered)
			{
				Background = FColor(0.16f, 0.16f, 0.19f);
				Border     = FColor(0.30f, 0.30f, 0.35f);
			}
		}

		FUSION_STYLE(FButton, "Button/Destructive", Shape, Background, Border)
		{
			Extends("Button/Base");
			Background = FColor(0.75f, 0.15f, 0.15f);
			Border     = FColor(0.60f, 0.10f, 0.10f);

			FUSION_ON(Hovered)
			{
				Background = FColor(0.88f, 0.22f, 0.22f);
				Border     = FColor(0.75f, 0.16f, 0.16f);
			}
			FUSION_ON(Pressed, Hovered)
			{
				Background = FColor(0.58f, 0.09f, 0.09f);
				Border     = FColor(0.45f, 0.06f, 0.06f);
			}
		}
	});

	app.CreateMainWindow<SampleWindow>();

	return app.Run();
}

