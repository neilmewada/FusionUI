
#include <Fusion/Core.h>
#include <Fusion/SDL3Platform.h>
#include <Fusion/VulkanRHI.h>
#include <Fusion/Widgets.h>

using namespace Fusion;

class SampleWindow : public FDecoratedWidget
{
	FUSION_CLASS(SampleWindow, FDecoratedWidget)
public:

	Ref<FVerticalStack> vstack;
	Ref<FHorizontalStack> hstack;

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
		.Thickness(3.0f);


		Child(
			FAssignNew(FVerticalStack, vstack)
			.ContentHAlign(EHAlign::Fill)
			.HAlign(EHAlign::Fill)
			.VAlign(EVAlign::Fill)
			.Padding(FMargin(5, 5, 5, 5))
			.Spacing(10)
			.Name("RootStack")
			(
				FNew(FWidget)
				.Height(25)
				.Name("Empty"),

				FAssignNew(FHorizontalStack, hstack)
				.ContentHAlign(EHAlign::Center)
				.ContentVAlign(EVAlign::Center)
				.Spacing(10)
				.ClipContent(true)
				.Name("hstack")
				(
					FNew(FDecoratedWidget)
					.FillRatio(1.0f)
					.Height(30)
					.Background(FBrush::Solid(FColors::Red)),

					FNew(FDecoratedWidget)
					.FillRatio(1.0f)
					.Height(30)
					.Background(FBrush::Solid(FColors::Green)),

					FNew(FDecoratedWidget)
					.FillRatio(1.0f)
					.Height(30)
					.Background(FBrush::Solid(FColors::Blue))
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
	
	void PaintOverlay(FPainter& painter) override
	{
		FPen pen;
	}
};

int main(int argc, char* argv[])
{
	FApplication app(argc, argv);

	app.CreateWindow<SampleWindow>();
	
	return app.Run();
}

