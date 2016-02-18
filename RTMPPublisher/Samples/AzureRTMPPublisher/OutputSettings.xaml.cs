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
using System.ComponentModel;
using Windows.Media.Capture;
using Windows.Media.MediaProperties;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Microsoft.Media.RTMP;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

namespace RTMPPublisher
{

  public class PublishProfileToVideoStringConverter : IValueConverter
  {
    public object Convert(object value, Type targetType, object parameter, string language)
    {
      PublishProfile prof = value as PublishProfile;
      return string.Format("{0} kbps, {1}x{2}, {3} fps", prof.TargetEncodingProfile.Video.Bitrate/1000, prof.TargetEncodingProfile.Video.Width, prof.TargetEncodingProfile.Video.Height, (prof.TargetEncodingProfile.Video.FrameRate.Numerator / prof.TargetEncodingProfile.Video.FrameRate.Denominator));
    
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
      throw new NotImplementedException();
    }
  }

  public class PublishProfileToAudioStringConverter : IValueConverter
  {
    public object Convert(object value, Type targetType, object parameter, string language)
    {
      PublishProfile prof = value as PublishProfile;
      return string.Format("{0} kbps, {1} kHz, {2}", prof.TargetEncodingProfile.Audio.Bitrate/1000, prof.TargetEncodingProfile.Audio.SampleRate/1000, prof.TargetEncodingProfile.Audio.ChannelCount >=2 ? "Stereo" : "Mono");
      
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
      throw new NotImplementedException();
    }
  }
  public sealed partial class OutputSettings : UserControl, INotifyPropertyChanged,ISettingsPane
  {

   

    public event EventHandler<EventArgs> OnDismissed;

    public OutputSettings()
    {
      this.InitializeComponent();
      Loaded += OutputSettings_Loaded;
    }

    private void OutputSettings_Loaded(object sender, RoutedEventArgs e)
    {
      SetData();
    }

    public event PropertyChangedEventHandler PropertyChanged;
    public SessionManager Manager
    {
      get
      {
        return _manager;
      }

      set
      {

        _manager = value;
        if (PropertyChanged != null)
          PropertyChanged(this, new PropertyChangedEventArgs("Manager"));
        SetData();
      }
    } 

    private void SetData(PublishProfile prof = null)
    {
      if (_manager == null) return;


      try {

        var selectedvidprops = Manager.CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties;
        var boundvidprof = prof != null && prof.TargetEncodingProfile != null && prof.TargetEncodingProfile.Video != null ? prof.TargetEncodingProfile.Video : selectedvidprops;

        var selectedaudprops = Manager.CurrentCapture.AudioDeviceController.GetMediaStreamProperties(MediaStreamType.Audio) as AudioEncodingProperties;
        var boundaudprof = prof != null && prof.TargetEncodingProfile != null && prof.TargetEncodingProfile.Audio != null ? prof.TargetEncodingProfile.Audio : selectedaudprops;

        double framesizeratio =  
          (double)boundvidprof.Width / (double)boundvidprof.Height; 
        if (framesizeratio == 4.0 / 3.0)
        {
          sldrWidth.StepFrequency = 4.0;
          sldrHeight.StepFrequency = 3.0;
        }
        else if (framesizeratio == 16.0 / 9.0)
        {
          sldrWidth.StepFrequency = 16.0;
          sldrHeight.StepFrequency = 9.0;
        }
        else if (framesizeratio == 3.0 / 2.0)
        {
          sldrWidth.StepFrequency = 3.0;
          sldrHeight.StepFrequency = 2.0;
        }
        else
        {
          sldrWidth.StepFrequency = framesizeratio;
          sldrHeight.StepFrequency = 1;
        }

        sldrWidth.Value = boundvidprof.Width;
        sldrHeight.Value = boundvidprof.Height;
        sldrBitrate.Value = boundvidprof.Bitrate / 1000;

        sldrKFInterval.Value = ((prof != null ? prof.KeyFrameInterval : 120) / (boundvidprof.FrameRate.Numerator / boundvidprof.FrameRate.Denominator));

        tbKFValue.Text = string.Format("{0:#####} frames/{1:####} secs",
         (prof != null ? prof.KeyFrameInterval : 120), sldrKFInterval.Value);

        sldrRTMPChunksize.Value = (prof != null ? prof.ClientChunkSize : 128);

        if (boundvidprof.ProfileId == H264ProfileIds.Baseline)
          cbxH264Profile.SelectedIndex = 0;
        else if (boundvidprof.ProfileId == H264ProfileIds.Main)
          cbxH264Profile.SelectedIndex = 1;
        else if (boundvidprof.ProfileId == H264ProfileIds.High)
          cbxH264Profile.SelectedIndex = 2;
        else
        {
          boundvidprof.ProfileId = H264ProfileIds.Main;
          cbxH264Profile.SelectedIndex = 1;
        }

        sldrAudioBitrate.Value = boundaudprof.Bitrate;
        cbxAudioSampling.SelectedIndex = boundaudprof.SampleRate == 48000 ? 0 : (boundaudprof.SampleRate == 44100 ? 1 : 2);
        tsAudioChannels.IsOn = boundaudprof.ChannelCount >= 2;

        tsLowLagBehavior.IsOn = (prof != null ? prof.EnableLowLatency : false);
        
      }
      catch
      {

      }
    }

    private void OnResolutionLockToggled(object sender, RoutedEventArgs e)
    {
      if (Manager == null) return;
      if (!tsResLockToInput.IsOn)
      {
        var selectedprops = lvPublishSettings.SelectedItem == null ?
          Manager.CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties
          : (lvPublishSettings.SelectedItem as PublishProfile).TargetEncodingProfile.Video;

        sldrWidth.Value = selectedprops.Width;
        sldrHeight.Value = selectedprops.Height;
        sldrWidth.IsEnabled = false;
        sldrHeight.IsEnabled = false;
      }
      else
      {
        sldrWidth.IsEnabled = true;
        sldrHeight.IsEnabled = true;
      }
    }
 

    private void btnCancel_Click(object sender, RoutedEventArgs e)
    {
      if (OnDismissed != null)
        OnDismissed(this, EventArgs.Empty);
    }
    private void sldrKFInterval_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
      if (Manager == null) return;
      var selectedprops = lvPublishSettings.SelectedItem == null ?
          Manager.CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties
          : (lvPublishSettings.SelectedItem as PublishProfile).TargetEncodingProfile.Video;
      tbKFValue.Text = string.Format("{0:#####} frames/{1:####} secs",
      (selectedprops.FrameRate.Numerator / selectedprops.FrameRate.Denominator) * sldrKFInterval.Value, sldrKFInterval.Value);
    }

    public void OnClose()
    {
      return;
    }

  

    private void btnRemovePublishSetting_Click(object sender, RoutedEventArgs e)
    {
      if ((sender as AppBarButton).Tag != null)
        _manager.PublishProfiles.Remove((sender as AppBarButton).Tag as PublishProfile);
    }

    private void btnAddPublishSetting_Click(object sender, RoutedEventArgs e)
    {
      try
      {
        var selectedprops = Manager.CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties;
        Manager.AddPublishProfile((uint)sldrWidth.Value,
          (uint)sldrHeight.Value,
          (uint)((selectedprops.FrameRate.Numerator / selectedprops.FrameRate.Denominator) * sldrKFInterval.Value),
          (uint)sldrRTMPChunksize.Value,
          (uint)(sldrBitrate.Value * 1000),
          cbxH264Profile.SelectedValue as string,
           (uint)(sldrAudioBitrate.Value * 1000),
           (cbxAudioSampling.SelectedIndex == 0 ? 48000U : (cbxAudioSampling.SelectedIndex == 1 ? 44100U : 32000U)),
           tsAudioChannels.IsOn);
        
      }
      catch
      {

      }
      
    }

    private SessionManager _manager = null;

    private void btnSavePublishSetting_Click(object sender, RoutedEventArgs e)
    {
      try
      {
        var selectedprops = Manager.CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties;
        Manager.AddPublishProfile((uint)sldrWidth.Value,
          (uint)sldrHeight.Value,
          (uint)((selectedprops.FrameRate.Numerator / selectedprops.FrameRate.Denominator) * sldrKFInterval.Value),
          (uint)sldrRTMPChunksize.Value,
          (uint)(sldrBitrate.Value * 1000),
          cbxH264Profile.SelectedValue as string , (uint)(sldrAudioBitrate.Value * 1000),
           (cbxAudioSampling.SelectedIndex == 0 ? 48000U : (cbxAudioSampling.SelectedIndex == 1 ? 44100U : 32000U)),
           tsAudioChannels.IsOn);
        return;
      }
      catch
      {

      }
    }

    //private void btnLoadPreset_Click(object sender, RoutedEventArgs e)
    //{

    //}

    //private void btnSavePreset_Click(object sender, RoutedEventArgs e)
    //{
      
    //}
  }
}
