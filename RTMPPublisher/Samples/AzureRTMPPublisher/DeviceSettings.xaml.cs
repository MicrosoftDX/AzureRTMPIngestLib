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
using System.Linq;
using Windows.Media.Capture;
using Windows.Media.MediaProperties;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

namespace RTMPPublisher
{


  public sealed partial class DeviceSettings : UserControl, INotifyPropertyChanged 
  {
    public event PropertyChangedEventHandler PropertyChanged;
    public event EventHandler<EventArgs> OnDismissed;
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
        Initialize();
      }
    }
    public DeviceSettings()
    {
      this.InitializeComponent();
    }


    private void Initialize()
    {
      cbxCamera.ItemsSource = Manager.Cameras;
      cbxMicrophone.ItemsSource = Manager.Microphones;
      cbxCamera.SelectedValue = Manager.Cameras.Where(d => d.Info.Id.ToLowerInvariant() == Manager.CurrentCapture.MediaCaptureSettings.VideoDeviceId.ToLowerInvariant()).FirstOrDefault();
      cbxMicrophone.SelectedValue = Manager.Microphones.Where(d => d.Info.Id.ToLowerInvariant() == Manager.CurrentCapture.MediaCaptureSettings.AudioDeviceId.ToLowerInvariant()).FirstOrDefault();
    }

    private void OnMicrophoneSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
      try {
        if (cbxMicrophone.SelectedValue != null)
        {
          DeviceWrapper dw = cbxMicrophone.SelectedValue as DeviceWrapper;
          cbxMicrophoneProfile.ItemsSource = dw.EncodingProperties;
          if (Manager.CurrentCapture.MediaCaptureSettings.AudioDeviceId.ToLowerInvariant() == dw.Info.Id.ToLowerInvariant()) //if this is the currently selected device
          {
            //selected the currently applied profile
            var curprofile = Manager.CurrentCapture.AudioDeviceController.GetMediaStreamProperties(MediaStreamType.Audio) as AudioEncodingProperties;
            cbxMicrophoneProfile.SelectedValue = dw.EncodingProperties.Where(ecw =>
            {
              return ecw.IsEqual(curprofile);
            }).FirstOrDefault();
          }
          else
          {
            if (dw.EncodingProperties.Count > 0)
              cbxMicrophoneProfile.SelectedIndex = dw.EncodingProperties.Count - 1;
          }
        }
      }
      catch
      {

      }
    }


    private void OnCameraSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
      try {
        if (cbxCamera.SelectedValue != null)
        {
          DeviceWrapper dw = cbxCamera.SelectedValue as DeviceWrapper;
          cbxCameraProfile.ItemsSource = dw.EncodingProperties;
          if (Manager.CurrentCapture.MediaCaptureSettings.VideoDeviceId.ToLowerInvariant() == dw.Info.Id.ToLowerInvariant()) //if this is the currently selected device
          {
            //selected the currently applied profile
            var curprofile = Manager.CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties;
            cbxCameraProfile.SelectedValue = dw.EncodingProperties.Where(ecw =>
            {
              return ecw.IsEqual(curprofile);
            }).FirstOrDefault();
          }
          else
          {
            if (dw.EncodingProperties.Count > 0)
              cbxCameraProfile.SelectedIndex = dw.EncodingProperties.Count - 1;
          }
        }
      }
      catch
      {

      }
    }

    private async void OnMicrophoneProfileSelectionChanged(object sender, SelectionChangedEventArgs e)
    { 
      
      DeviceWrapper dwMicrophone = null;
      
      EncodingPropertyWrapper audprops = null;

      try
      {
        if (cbxMicrophone.SelectedValue != null && cbxMicrophoneProfile.SelectedValue != null)
        {
          dwMicrophone = cbxMicrophone.SelectedValue as DeviceWrapper;
          audprops = (cbxMicrophoneProfile.SelectedValue as EncodingPropertyWrapper);
          var curprofile = Manager.CurrentCapture.AudioDeviceController.GetMediaStreamProperties(MediaStreamType.Audio) as AudioEncodingProperties;
          if ((Manager.CurrentCapture.MediaCaptureSettings.AudioDeviceId.ToLowerInvariant() == dwMicrophone.Info.Id.ToLowerInvariant() && audprops.IsEqual(curprofile) == false) ||
              (Manager.CurrentCapture.MediaCaptureSettings.AudioDeviceId.ToLowerInvariant() != dwMicrophone.Info.Id.ToLowerInvariant()))
          {
            await Manager.RestartCapturePreviewAsync(dwMicrophone.Info.Id,
              Manager.CurrentCapture.MediaCaptureSettings.VideoDeviceId,
              audprops.Data as AudioEncodingProperties,
              (cbxCameraProfile.SelectedValue as EncodingPropertyWrapper).Data as VideoEncodingProperties);
          }
        }
      }
      catch
      {

      }

      
    }

    private async void OnCameraProfileSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
      DeviceWrapper dwCamera = null;
     
      EncodingPropertyWrapper vidprops = null;

      try {
        if (cbxCamera.SelectedValue != null && cbxCameraProfile.SelectedValue != null)
        {
          dwCamera = cbxCamera.SelectedValue as DeviceWrapper;
          vidprops = (cbxCameraProfile.SelectedValue as EncodingPropertyWrapper);
          var curprofile = Manager.CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties;
          if ((Manager.CurrentCapture.MediaCaptureSettings.VideoDeviceId.ToLowerInvariant() == dwCamera.Info.Id.ToLowerInvariant() && vidprops.IsEqual(curprofile) == false) ||
              (Manager.CurrentCapture.MediaCaptureSettings.VideoDeviceId.ToLowerInvariant() != dwCamera.Info.Id.ToLowerInvariant()))
          {
            await Manager.RestartCapturePreviewAsync(Manager.CurrentCapture.MediaCaptureSettings.AudioDeviceId, dwCamera.Info.Id,
              (cbxMicrophoneProfile.SelectedValue as EncodingPropertyWrapper).Data as AudioEncodingProperties,
              vidprops.Data as VideoEncodingProperties);
          }
        }
      }
      catch
      {

      }
   
       

    }

    private void btnDismiss_Click(object sender, RoutedEventArgs e)
    {
    
      if (OnDismissed != null)
        OnDismissed(this, EventArgs.Empty);
    }

    //public void OnClose()
    //{
    //  bool vidchanged = false;
    //  bool audchanged = false;
    //  DeviceWrapper dwCamera = null;
    //  DeviceWrapper dwMicrophone = null;
    //  EncodingPropertyWrapper vidprops = null;
    //  EncodingPropertyWrapper audprops = null;
    //  if (cbxCamera.SelectedValue != null && cbxCameraProfile.SelectedValue != null)
    //  {
    //    dwCamera = cbxCamera.SelectedValue as DeviceWrapper;
    //    vidprops = (cbxCameraProfile.SelectedValue as EncodingPropertyWrapper);
    //    var curprofile = Manager.CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties;
    //    if ((Manager.CurrentCapture.MediaCaptureSettings.VideoDeviceId.ToLowerInvariant() == dwCamera.Info.Id.ToLowerInvariant() && vidprops.IsEqual(curprofile) == false) ||
    //        (Manager.CurrentCapture.MediaCaptureSettings.VideoDeviceId.ToLowerInvariant() != dwCamera.Info.Id.ToLowerInvariant()))
    //    {
    //      vidchanged = true;
    //    }
    //  }
    //  if (cbxMicrophone.SelectedValue != null && cbxMicrophoneProfile.SelectedValue != null)
    //  {
    //    dwMicrophone = cbxMicrophone.SelectedValue as DeviceWrapper;
    //    audprops = (cbxMicrophoneProfile.SelectedValue as EncodingPropertyWrapper);
    //    var curprofile = Manager.CurrentCapture.AudioDeviceController.GetMediaStreamProperties(MediaStreamType.Audio) as AudioEncodingProperties;
    //    if ((Manager.CurrentCapture.MediaCaptureSettings.AudioDeviceId.ToLowerInvariant() == dwMicrophone.Info.Id.ToLowerInvariant() && audprops.IsEqual(curprofile) == false) ||
    //        (Manager.CurrentCapture.MediaCaptureSettings.AudioDeviceId.ToLowerInvariant() != dwMicrophone.Info.Id.ToLowerInvariant()))
    //    {
    //      audchanged = true;
    //    }
    //  }

    //  if (vidchanged && audchanged)
    //  {
    //    Manager.RestartCapturePreview(dwMicrophone.Info.Id, dwCamera.Info.Id, audprops.Data as AudioEncodingProperties, vidprops.Data as VideoEncodingProperties);
    //  }
    //  else if (vidchanged)
    //  {
    //    Manager.RestartCapturePreview(dwMicrophone.Info.Id, dwCamera.Info.Id, null, vidprops.Data as VideoEncodingProperties);
    //  }
    //  else if (audchanged)
    //  {
    //    Manager.RestartCapturePreview(dwMicrophone.Info.Id, dwCamera.Info.Id, audprops.Data as AudioEncodingProperties);
    //  }

    //}

    private SessionManager _manager = null;


  }
}
