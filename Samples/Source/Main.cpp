#include <Fusion/Core.h>
#include <Fusion/Widgets.h>

using namespace Fusion;

class SampleWindow : public FDecoratedBox
{
	FUSION_WIDGET(SampleWindow, FDecoratedBox)
public:

	Ref<FVerticalStack> vstack;
	Ref<FHorizontalStack> hstack;
	Ref<FDecoratedBox> gradientBorder;
	Ref<FDecoratedBox> gradientWidget;

	Ref<FTextButton> btn0;
	Ref<FTimer> timer;

	void Construct() override
	{
		Super::Construct();

		FGradient gradient = FGradient::Linear(FMath::Deg2Rad(45.0f))
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
		.GradientSpace(EGradientSpace::ArcLength)
		.GradientOffset(0.0f);


		Child(
			FAssignNew(FVerticalStack, vstack)
			.ContentHAlign(EHAlign::Fill)
			.HAlign(EHAlign::Fill)
			.VAlign(EVAlign::Fill)
			.Spacing(10)
			.Name("RootStack")
			(
				FNew(FTextButton)
				.Height(32)
				.Style("Button/Primary")
				.Text("Toggle Enable/Disable")
				.OnClick([this]
				{
					hstack->Enabled(!hstack->Enabled());
				}),

				FAssignNew(FDecoratedBox, gradientBorder)
				.Border(gradientPen)
				.Background(FColors::White)
				.Shape(FRoundedRectangle(5.0f))
				.Height(35)
				.Name("GradientBorder"),

				FAssignNew(FHorizontalStack, hstack)
				.ContentHAlign(EHAlign::Center)
				.ContentVAlign(EVAlign::Center)
				.Spacing(10)
				.ClipContent(true)
				.Name("hstack")
				(
					FAssignNew(FTextButton, btn0)
					.FillRatio(1.0f)
					.Height(32)
					.Style("Button/Primary")
					.Text("Primary")
					.OnClick([this, gradientPen]
					{
						FUSION_LOG_INFO("Debug", "Primary clicked!");

						FPen toPen = gradientPen;
						toPen.GradientOffset(1.0f);

						FAnimate_Tween(gradientBorder, Border)
						.Duration(5.0f)
						.Loop(EAnimationLoopMode::Loop)
						.Easing(EEasingType::Linear)
						.From(gradientPen)
						.To(toPen)
						.Play();

						FAnimate_Tween(this, GradientOffset)
						.Duration(3.0f)
						.Loop(EAnimationLoopMode::Loop)
						.Easing(EEasingType::Linear)
						.From(0.0f)
						.To(1.0f)
						.Play();
					}),

					FNew(FTextButton)
					.FillRatio(1.0f)
					.Height(32)
					.Text("Secondary")
					.Style("Button/Secondary")
					.OnClick([this]
					{
						FUSION_LOG_INFO("Debug", "Secondary clicked!");

						FAnimate_Spring(gradientBorder, Transform)
						.Target(!rotated ? FAffineTransform::Rotation(FMath::Deg2Rad(90)) : FAffineTransform::Identity())
						.Play();

						rotated = !rotated;
					}),

					FNew(FTextButton)
					.FillRatio(1.0f)
					.Height(32)
					.Text("Destructive")
					.Style("Button/Destructive")
					.OnClick([this, gradient]
					{
						FUSION_LOG_INFO("Debug", "Destructive clicked!");

						FBrush altGradient = FGradient::Linear(FMath::Deg2Rad(45 + 180))
							.AddStop(FColor(0.00f, 0.08f, 0.30f), 0.0f)   // deep navy
							.AddStop(FColor(0.00f, 0.45f, 0.65f), 0.35f)  // ocean blue
							.AddStop(FColor(0.00f, 0.75f, 0.55f), 0.65f)  // teal
							.AddStop(FColor(0.20f, 0.95f, 0.50f), 1.0f);  // bright mint

						FAnimate_Spring(gradientWidget, Background)
						.Target(gradientToggled ? gradient : altGradient)
						.Stiffness(80.0f)
						.Damping(12.0f)
						.Play();

						gradientToggled = !gradientToggled;
					}),

					FNew(FTextInput)
					.Style("TextInput/Base")
					.Placeholder("Type here...")
					.FillRatio(1.0f)
					.OnTextChanged([this](const FString& text)
					{
						if (text == "disable")
						{
							hstack->Enabled(false);
						}
					})
				),

				FAssignNew(FDecoratedBox, gradientWidget)
				.Background(gradient)
				.Shape(FRoundedRectangle(5.0f))
				.Height(100),

				FNew(FDecoratedBox)
				.Background(FBrush::Image("embed:/Icons/TransparentPattern.png").BrushTiling(EBrushTiling::TileXY).ImageFit(EImageFit::Fill).BrushSize(FVec2(1, 1) * 32))
				.Shape(FRoundedRectangle(5.0f))
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

	void PaintOverContent(FPainter& painter) override
	{
		return;

		f32 go = GradientOffset();

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
		.GradientOffset(go)
		.DashGap(DashGap())
		.DashLength(DashLength())
		.DashGap(DashGap())
		.DashPhase(DashPhase())
		.Style(EPenStyle::Dashed)
		.Thickness(3.0f)
		.GradientSpace(EGradientSpace::ArcLength);

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

		painter.SetFont(
			FFont::Regular(FFont::kDefaultFamilyName, 10)
			.Style(EFontStyle::Normal)
			.Weight(EFontWeight::Regular)
		);

		painter.SetPen(FPen::Solid(FColors::White));

		const FString text = "Hello World, Good bye!";

		painter.DrawText(FVec2(50, 300), text);

		painter.SetFont(
			FFont::Regular(FFont::kDefaultFamilyName, 20)
			.Style(EFontStyle::Normal)
			.Weight(EFontWeight::Regular)
		);

		painter.DrawText(FVec2(120, 300), text);

        painter.SetFont(
            FFont::Regular(FFont::kDefaultFamilyName, 48)
            .Style(EFontStyle::Normal)
            .Weight(EFontWeight::Regular)
        );

        painter.DrawText(FVec2(300, 300), text);
	}

	FUSION_PROPERTY(f32, GradientOffset);
	FUSION_PROPERTY(f32, DashLength);
	FUSION_PROPERTY(f32, DashGap);
	FUSION_PROPERTY(f32, DashPhase);

	bool rotated = false;
	bool gradientToggled = false;
};

int main(int argc, char* argv[])
{
	FApplication app(argc, argv);

	Ref<FTheme> theme = app.CreateDefaultTheme();

	theme->MergeStyleSheet(FUSION_STYLE_SHEET
	{
		FColor WindowBackgroundColor = FColor(0.13f, 0.13f, 0.15f);
		FPen   FocusOutline			 = FPen::Solid(FColor(0.47f, 0.73f, 1.0f, 0.85f)).Thickness(2.0f);

		FUSION_STYLE(SampleWindow, "SampleWindow", Background, Padding)
		{
			Background = WindowBackgroundColor;
			Padding	   = FMargin(1, 1, 1, 1) * 5;
		}

		FUSION_STYLE(FDecoratedBox, "Base/FocusRing", Outline, OutlineOffset)
		{
			OutlineOffset = 0;

			Transition(Outline,			FTransition::MakeTween(0.15f));
			Transition(OutlineOffset,	FTransition::MakeTween(0.3f));

			FUSION_ON(FocusVisible)
			{
				Outline = FocusOutline;
				OutlineOffset = 2.5f;
			}
		}

		FUSION_STYLE(FButton, "Button/Base", Shape, Background, Border, Outline, OutlineOffset)
		{
			Extends("Base/FocusRing");
			Shape = FRoundedRectangle(5.0f);

			Transition(Background, FTransition::MakeTween(0.1f));

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

		FUSION_STYLE(FTextInput, "TextInput/Base", Shape,
			Background, Border, Outline, Padding, Font,
			TextColor, PlaceholderColor, SelectionColor, CursorColor)
		{
			Extends("Base/FocusRing");

			Shape            = FRoundedRectangle(5.0f);
			Background       = FColor(0.10f, 0.10f, 0.12f);
			Border           = FColor(0.30f, 0.30f, 0.34f);
			Padding          = FMargin(8, 6, 8, 6);
			Font             = FFont::Regular(FFont::kDefaultFamilyName, 14);
			TextColor        = FColors::White;
			PlaceholderColor = FColor(0.45f, 0.45f, 0.50f);
			SelectionColor   = FColor(0.23f, 0.51f, 0.96f, 0.45f);
			CursorColor      = FColors::White;

			FUSION_ON(Focused)
			{
				Border = FColor(0.47f, 0.73f, 1.0f, 0.85f);
			}

			FUSION_ON(FocusVisible)
			{
				Outline = FocusOutline;
			}

			FUSION_ON(Hovered)
			{
				Border = FColor(0.45f, 0.45f, 0.50f);
			}

			FUSION_ON(Disabled)
			{
				Background       = FColor(0.08f, 0.08f, 0.10f);
				Border           = FColor(0.20f, 0.20f, 0.23f);
				TextColor        = FColor(0.35f, 0.35f, 0.38f);
				PlaceholderColor = FColor(0.25f, 0.25f, 0.28f);
				CursorColor      = FColor(0.35f, 0.35f, 0.38f);
			}
		}
	});

	app.CreateMainWindow<SampleWindow>();

#if FUSION_PLATFORM_MAC
	app.SetInitialWindowSize(800, 600);
#else
	app.SetInitialWindowSize(1200, 900);
#endif

	return app.Run();
}

