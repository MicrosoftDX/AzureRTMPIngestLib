/****************************************************************************************************************************

RTMP Live Publishing Library

Copyright (c) Microsoft Corporation

All rights reserved.

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation 
files (the ""Software""), to deal in the Software without restriction, including without limitation the rights to use, copy, 
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software 
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


*****************************************************************************************************************************/

using System;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Windows.Media.Capture;
using Windows.Media.Streaming.Adaptive;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Navigation;
using Microsoft.Media.RTMP;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

namespace RTMPPublisher
{
  public sealed partial class PublishView : Page
  {


    RTMPPublishSession _publishsession = null;
    Channel _targetChannel = null;
    SessionManager _deviceManager = null;
    TimeSpan sessionduration = TimeSpan.FromSeconds(0);
    System.Threading.Timer _clockTimer = null;
    LowLagMediaRecording _lowlagCapture = null;

    public void SetTargetChannel(Channel c)
    {

      _targetChannel = c;
      tbChannel.Text = c.Name + " - " + c.Description;

    }
    public async Task ApplyTargetChannels()
    {
      if (_deviceManager == null)
        return;

      var epuri = _targetChannel.Input.Endpoints.Where(iep => iep.Protocol == "RTMP").First().Url;
      var endpointhistory = _deviceManager.AddIngestURLToHistory(epuri, "AZURE");
      foreach (var prof in _deviceManager.PublishProfiles)
      {
        prof.EndpointUri = epuri;
        if (endpointhistory != null && _deviceManager.UseContinuousTimestamps)
        {
          prof.AudioTimestampBase = endpointhistory.LastAudioTS + 40000000; //add a 4 second offset per AMS PG suggestion
          prof.VideoTimestampBase = endpointhistory.LastVideoTS + 40000000;
        }
        else
        {
          prof.AudioTimestampBase = -1;
          prof.VideoTimestampBase = -1;
        }
      }

      _deviceManager.StreamURL = _targetChannel.Preview.Endpoints.Where(pep => pep.Protocol == "FragmentedMP4").First().Url;
      _deviceManager.AddStreamURLToHistory(_deviceManager.StreamURL, "SMOOTH");

      _deviceManager.StreamType = "SMOOTH";


    }

    public PublishView()
    {
      this.InitializeComponent();

      Window.Current.Activated += Current_Activated;
    }

    protected async override void OnNavigatedTo(NavigationEventArgs e)
    {
      _deviceManager = (Application.Current as App).DeviceManager;

      await _deviceManager.InitializeAsync(ceSourcePreview);
      await StartPreviewAsync();

      if (e.Parameter != null && e.Parameter is Channel)
      {
        this.SetTargetChannel(e.Parameter as Channel);
      }
    }

    protected async override void OnNavigatingFrom(NavigatingCancelEventArgs e)
    {
      _targetChannel = null;
      btnPublish_Unchecked(null, null);
    }
    private void Current_Activated(object sender, Windows.UI.Core.WindowActivatedEventArgs e)
    {
      if (e.WindowActivationState == Windows.UI.Core.CoreWindowActivationState.Deactivated)
        DismissSettings();
    }

    private void btnSettings_Click(object sender, RoutedEventArgs e)
    {
      VisualStateManager.GoToState(this, "ShowRightPanelSettingsLauncher", true);
    }

    private void SettingsLauncher_OnSettingsSelectionChanged(int SelectionIndex)
    {
      switch (SelectionIndex)
      {
        case 0:
          VisualStateManager.GoToState(this, "ShowRightPanelDeviceSettings", true);
          break;
        case 1:
          VisualStateManager.GoToState(this, "ShowRightPanelOutputSettings", true);
          break;
        //case 2:
        //  VisualStateManager.GoToState(this, "ShowRightPanelStreamSettings", true);
        //  break;
        default:
          break;
      }
    }

    private void OnSettingsDismissed(object sender, EventArgs e)
    {
      if (sender == ccSettingsLauncher)
        DismissSettings();
      else
      {
        VisualStateManager.GoToState(this, "ShowRightPanelSettingsLauncher", true);
      }
    }

    private async void btnPublish_Checked(object sender, RoutedEventArgs e)
    {
      try
      {

        await ApplyTargetChannels();
        //RTMP Publish specific Code 

        var inputprof = _deviceManager.GetInputProfile();

        _publishsession = new RTMPPublishSession(_deviceManager.PublishProfiles);

        //handle events (if needed)
        _publishsession.PublishFailed += OnPublishFailed;
        _publishsession.SessionClosed += OnSessionClosed;
        //get the sink
        var sink = await _publishsession.GetCaptureSinkAsync();

        if (_deviceManager.EnableLowLatency)
        {
          _lowlagCapture = await _deviceManager.CurrentCapture.PrepareLowLagRecordToCustomSinkAsync(inputprof, sink);
          await _lowlagCapture.StartAsync();
        }
        else
        await _deviceManager.CurrentCapture.StartRecordToCustomSinkAsync(inputprof, sink);



        _deviceManager.IsPublishing = true;
        sessionduration = TimeSpan.FromSeconds(0);
        _clockTimer = new Timer(new TimerCallback((o) =>
          {
            this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
             {
               sessionduration = sessionduration.Add(TimeSpan.FromMilliseconds(500));
               tbClock.Text = sessionduration.ToString(@"hh\:mm\:ss\:fff");
               if (sessionduration.Seconds >= 30) //need at least 30 seconds of content published before streaming
                 btnStream.IsEnabled = true;
             });
          }), null, 0, 500);
      }
      catch (Exception Ex)
      {
                System.Diagnostics.Debug.WriteLine("Error starting recording: {0}", Ex);
      }
    }

    private async void btnPublish_Unchecked(object sender, RoutedEventArgs e)
    {
      try
      {
        if (_clockTimer != null)
          _clockTimer.Change(Timeout.Infinite, Timeout.Infinite);
        _clockTimer = null;
        tbClock.Text = "";
        btnStream.IsEnabled = false;
        if (_deviceManager.IsPublishing)
        {
          if (_deviceManager.EnableLowLatency && _lowlagCapture != null)
            await _lowlagCapture.FinishAsync();
          else
          await _deviceManager.CurrentCapture.StopRecordAsync();
        }

        await _deviceManager.SaveHistoryAsync();
      }
      catch (Exception Ex)
      {

      }

      _deviceManager.IsPublishing = false;
    }

    private async void btnStream_Checked(object sender, RoutedEventArgs e)
    {
      try
      {
        if (_deviceManager.StreamType == "DASH" || _deviceManager.StreamType == "HLS")
        {
          var result = await AdaptiveMediaSource.CreateFromUriAsync(new Uri(_deviceManager.StreamURL));
          if (result.Status == AdaptiveMediaSourceCreationStatus.Success)
          {
            mpStreamPreview.SetMediaStreamSource(result.MediaSource);
          }
        }
        else
        {
          mpStreamPreview.Source = new Uri(_deviceManager.StreamURL);
        }

        mpStreamPreview.Visibility = Visibility.Visible;


      }
      catch
      {

      }
    }

    private void btnStream_Unchecked(object sender, RoutedEventArgs e)
    {
      mpStreamPreview.Stop();
      mpStreamPreview.Source = null;
      mpStreamPreview.Visibility = Visibility.Collapsed;
    }

    private void SettingsDismiss_PointerPressed(object sender, PointerRoutedEventArgs e)
    {
      DismissSettings();

    }

    private void DismissSettings()
    {

      ccOutputSettings.OnClose();
      VisualStateManager.GoToState(this, "HideSettings", true);
    }


    public async Task StartPreviewAsync()
    {
      if (_deviceManager.IsPreviewing == false)
      {
        await _deviceManager.RestartCapturePreviewAsync(null, null);
      }

      if (ccDeviceSettings.Manager == null)
        ccDeviceSettings.Manager = _deviceManager;
      if (ccOutputSettings.Manager == null)
        ccOutputSettings.Manager = _deviceManager;
    }


    private async void OnSessionClosed(RTMPPublishSession sender, SessionClosedEventArgs args)
    {

      await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, new Windows.UI.Core.DispatchedHandler(() =>
      {
        try
        {

          if (_deviceManager.History != null)
            _deviceManager.AddIngestURLToHistory(_deviceManager.PublishProfiles[0].EndpointUri, "AZURE", args.LastVideoTimestamp, args.LastAudioTimestamp);
        }
        catch
        {

        }
        btnStream.IsChecked = false;

      }));
    }

    private async void OnPublishFailed(RTMPPublishSession sender, FailureEventArgs args)
    {
      return;
    }


    private void btnBack_Click(object sender, RoutedEventArgs e)
    {
      btnPublish_Unchecked(null, null);
      this.Frame.GoBack();
    }


  }
}
