#include "SimpleSavingLoadingScreen.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "MoviePlayer.h"
#include "Widgets/Images/SThrobber.h"

//=========================================================================================================
// 
//=========================================================================================================
struct FSimpleSavingLoadingScreenBrush : public FSlateDynamicImageBrush, public FGCObject
{
	//
	FSimpleSavingLoadingScreenBrush(const FName InTextureName, const FVector2D& InImageSize) : FSlateDynamicImageBrush(InTextureName, InImageSize)
	{
		SetResourceObject(LoadObject<UObject>(NULL, *InTextureName.ToString()));
	}

	//
	virtual void AddReferencedObjects(FReferenceCollector& Collector)
	{
		if (UObject* CachedResourceObject = GetResourceObject())
		{
			Collector.AddReferencedObject(CachedResourceObject);
		}
	}
};

static FText _LoadingScreenStatus = FText::GetEmpty();

static TArray<class UMaterialInterface*> _LoadingScreenIcons = {};

//=========================================================================================================
// 
//=========================================================================================================
static FText GetLoadingScreenStatus()
{
	return _LoadingScreenStatus;
}

//=========================================================================================================
// 
//=========================================================================================================
class SSimpleSavingLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSimpleSavingLoadingScreen) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		if ( GEngine != NULL && GEngine->GameViewport != NULL)
		{
			FWidgetActiveTimerDelegate Delegate;
			RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SSimpleSavingLoadingScreen::TimerTest));

			// Load version of the logo with text baked in, path is hardcoded because this loads very early in startup
			static const FName LoadingScreenName(TEXT("/Game/M_SimpleSavingLoadingScreen.M_SimpleSavingLoadingScreen")); 
			int32 PictureWide = 1920;
			int32 PictureTall = 1080;

			float flPictureAspectRatio = (float)PictureWide / (float)PictureTall;
				
			FVector2D ScreenSize;
			GEngine->GameViewport->GetViewportSize(ScreenSize);

			int32 Wide = ScreenSize.Y * flPictureAspectRatio;
			int32 Tall = ScreenSize.Y;

			LoadingScreenBrush = MakeShareable(new FSimpleSavingLoadingScreenBrush(LoadingScreenName, FVector2D(Wide, Tall)));
		
			TSharedPtr<SHorizontalBox> HorizontalBox;

			FSlateBrush *BGBrush = new FSlateBrush();
			BGBrush->TintColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);

			ChildSlot
				[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SBorder)	
					.BorderImage(BGBrush)
				]
				+SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(LoadingScreenBrush.Get())
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(HorizontalBox, SHorizontalBox)
				]

				+SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Bottom)
				[
					SNew(STextBlock)
					.Text_Static(GetLoadingScreenStatus)
				]

				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.VAlign(VAlign_Bottom)
					.HAlign(HAlign_Right)
					.Padding(FMargin(10.0f))
					[
						SNew(SThrobber)
						.Visibility(this, &SSimpleSavingLoadingScreen::GetLoadIndicatorVisibility)
					]
				]
			];

			//
			//IconBrushes.SetNum(_LoadingScreenIcons.Num());
			for (int32 i=0; i<_LoadingScreenIcons.Num(); i++)
			{
				FSlateBrush* IconBrush = new FSlateBrush();
				IconBrush->TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
				IconBrush->SetResourceObject(_LoadingScreenIcons.GetData()[i]);
				//IconBrushes.GetData()[i] = MakeShareable(new FSimpleSavingLoadingScreenBrush(_LoadingScreenIcons.GetData()[i]->GetFName(), FVector2D(100, 100)));

				HorizontalBox->AddSlot()
				.VAlign(VAlign_Center)
				.Padding(5.0f)
				[
					SNew(SBox)
					.WidthOverride(200)
					.HeightOverride(200)
					[
						SNew(SImage)
						.Image(IconBrush) //IconBrushes.GetData()[i].Get())
					]
				];
			}
		}
	}

	EActiveTimerReturnType TimerTest(double InCurrentTime, float InDeltaTime)
	{
		return EActiveTimerReturnType::Continue;
	}

private:

	/** Rather to show the ... indicator */
	EVisibility GetLoadIndicatorVisibility() const
	{
		bool Vis =  GetMoviePlayer()->IsLoadingFinished();
		return GetMoviePlayer()->IsLoadingFinished() ? EVisibility::Collapsed : EVisibility::Visible;
	}
	
	/** Loading screen image brush */
	TSharedPtr<FSlateDynamicImageBrush> LoadingScreenBrush;

	//TArray<TSharedPtr<FSlateDynamicImageBrush>> IconBrushes;
};

//===============================================================================================================================
// 
//===============================================================================================================================
class FSimpleSavingLoadingScreenModule : public ISimpleSavingLoadingScreenModule
{
public:
	virtual void StartupModule() override
	{
		// Force load for cooker reference
		LoadObject<UObject>(nullptr, TEXT("/Game/M_SimpleSavingLoadingScreen.M_SimpleSavingLoadingScreen"));

		if (IsMoviePlayerEnabled())
		{
			CreateScreen();
		}
	}
	
	virtual bool IsGameModule() const override
	{
		return true;
	}

	virtual void StartInGameLoadingScreen(bool bPlayUntilStopped, float PlayTime, bool Play) override
	{
		_LoadingScreenStatus = FText::FromString(TEXT(""));

		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = !bPlayUntilStopped;
		LoadingScreen.bWaitForManualStop = bPlayUntilStopped;
		LoadingScreen.bAllowEngineTick = bPlayUntilStopped;
		LoadingScreen.MinimumLoadingScreenDisplayTime = PlayTime;
		LoadingScreen.WidgetLoadingScreen = SNew(SSimpleSavingLoadingScreen);
		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);

		if (Play)
		{
			GetMoviePlayer()->PlayMovie();
		}
	}

	//
	virtual void WaitForMovieToFinish() override
	{
		GetMoviePlayer()->WaitForMovieToFinish(true);
	}

	//
	virtual void SetIcons(TArray<class UMaterialInterface*> InIcons) override
	{
		_LoadingScreenIcons = MoveTemp(InIcons);
	}

	//
	virtual void SetLoadingScreenStatus(const FText &InText) override
	{
		_LoadingScreenStatus = InText;
	}

	virtual FText GetLoadingScreenStatus() const override
	{
		return _LoadingScreenStatus;
	}

	virtual void StopInGameLoadingScreen() override
	{
		GetMoviePlayer()->StopMovie();
	}

	virtual void CreateScreen()
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
		LoadingScreen.MinimumLoadingScreenDisplayTime = 3.f;
		LoadingScreen.WidgetLoadingScreen = SNew(SSimpleSavingLoadingScreen);
		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
};

IMPLEMENT_GAME_MODULE(FSimpleSavingLoadingScreenModule, SimpleSavingLoadingScreen);
