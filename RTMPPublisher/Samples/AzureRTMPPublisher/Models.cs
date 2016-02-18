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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.Serialization;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;
using Windows.Media.Capture;
using Windows.Media.MediaProperties;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;

namespace RTMPPublisher
{

  public class DeviceWrapper : INotifyPropertyChanged
  {
    private DeviceInformation _di = null;
    private BitmapImage _bi = null;
    private List<EncodingPropertyWrapper> _encodingprops = null;
    private DeviceClass _class;
    public event PropertyChangedEventHandler PropertyChanged;
    public DeviceWrapper(DeviceInformation di, DeviceClass dclass)
    {
      _di = di;
      _class = dclass;

    }

    public async Task LoadThumbnailAsync()
    {
      var dt = await _di.GetThumbnailAsync();
      _bi = new BitmapImage();
      _bi.SetSource(dt);
      if (PropertyChanged != null)
        PropertyChanged(this, new PropertyChangedEventArgs("Thumbnail"));
    }

    public async Task LoadEncodingPropertiesAsync()
    {
      MediaCapture capture = new MediaCapture();
      MediaCaptureInitializationSettings settings = new MediaCaptureInitializationSettings();
      if (_class == DeviceClass.VideoCapture)
        settings.VideoDeviceId = _di.Id;
      else
        settings.AudioDeviceId = _di.Id;

      await capture.InitializeAsync(settings);

      if (_class == DeviceClass.VideoCapture)
      {
        _encodingprops = capture.VideoDeviceController.GetAvailableMediaStreamProperties(MediaStreamType.VideoRecord).Where(p => p is VideoEncodingProperties && 
        (p as VideoEncodingProperties).FrameRate.Numerator / (p as VideoEncodingProperties).FrameRate.Denominator >= 10 &&
        (p as VideoEncodingProperties).Width >= 320).OrderBy(p => (p as VideoEncodingProperties).Width).Select(p => new EncodingPropertyWrapper(p, this)).ToList();
        if (_encodingprops.Count == 0)
          _encodingprops.Add(new EncodingPropertyWrapper(capture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord), this));
      }
      else
      {
        _encodingprops = capture.AudioDeviceController.GetAvailableMediaStreamProperties(MediaStreamType.Audio).Where(p => p is AudioEncodingProperties).Select(p => new EncodingPropertyWrapper(p, this)).ToList();
        if (_encodingprops.Count == 0)
          _encodingprops.Add(new EncodingPropertyWrapper(capture.AudioDeviceController.GetMediaStreamProperties(MediaStreamType.Audio), this));


      }
    }
    public DeviceInformation Info
    {
      get
      {
        return _di;
      }
    }

    public ImageSource Thumbnail
    {
      get
      {
        if (_bi == null)
          LoadThumbnailAsync();
        return _bi;
      }
    }

    public List<EncodingPropertyWrapper> EncodingProperties
    {
      get
      {

        return _encodingprops;
      }
    }


  }

  public class EncodingPropertyWrapper
  {
    IMediaEncodingProperties _props;
    DeviceWrapper _devicewrapper = null;
    public EncodingPropertyWrapper(IMediaEncodingProperties props, DeviceWrapper dw)
    {
      _props = props;
    }

    public bool IsEqual(VideoEncodingProperties props)
    {
      var curprofile = _props as VideoEncodingProperties;
      if(curprofile == null) return false;
      if (props == null) return false;
      return (props.Bitrate == curprofile.Bitrate &&
           ((props.FrameRate != null && curprofile.FrameRate != null &&
           props.FrameRate.Numerator == curprofile.FrameRate.Numerator &&
           props.FrameRate.Denominator == curprofile.FrameRate.Denominator) || (props.FrameRate == null && curprofile.FrameRate == null)) &&
           ((props.PixelAspectRatio != null && curprofile.PixelAspectRatio != null && props.PixelAspectRatio.Numerator == curprofile.PixelAspectRatio.Numerator
           && props.PixelAspectRatio.Denominator == curprofile.PixelAspectRatio.Denominator)
           || (props.PixelAspectRatio == null && curprofile.PixelAspectRatio == null))
           && props.Height == curprofile.Height && props.Width == curprofile.Width
           && props.ProfileId == curprofile.ProfileId && props.Subtype == curprofile.Subtype);
    }

    public bool IsEqual(AudioEncodingProperties props)
    {
      var curprofile = _props as AudioEncodingProperties;
      if (curprofile == null) return false;
      if (props == null) return false;

      return (props.Bitrate == curprofile.Bitrate && props.BitsPerSample == curprofile.BitsPerSample &&
            props.ChannelCount == curprofile.ChannelCount && props.SampleRate == curprofile.SampleRate &&
            props.Subtype == curprofile.Subtype);
    }

    public static bool IsEqual(VideoEncodingProperties props, VideoEncodingProperties curprofile)
    {
      
      if (curprofile == null) return false;
      if (props == null) return false;
      return (props.Bitrate == curprofile.Bitrate &&
           ((props.FrameRate != null && curprofile.FrameRate != null &&
           props.FrameRate.Numerator == curprofile.FrameRate.Numerator &&
           props.FrameRate.Denominator == curprofile.FrameRate.Denominator) || (props.FrameRate == null && curprofile.FrameRate == null)) &&
           ((props.PixelAspectRatio != null && curprofile.PixelAspectRatio != null && props.PixelAspectRatio.Numerator == curprofile.PixelAspectRatio.Numerator
           && props.PixelAspectRatio.Denominator == curprofile.PixelAspectRatio.Denominator)
           || (props.PixelAspectRatio == null && curprofile.PixelAspectRatio == null))
           && props.Height == curprofile.Height && props.Width == curprofile.Width
           && props.ProfileId == curprofile.ProfileId && props.Subtype == curprofile.Subtype);
    }

    public static bool IsEqual(AudioEncodingProperties props,AudioEncodingProperties curprofile)
    {
      
      if (curprofile == null) return false;
      if (props == null) return false;

      return (props.Bitrate == curprofile.Bitrate && props.BitsPerSample == curprofile.BitsPerSample &&
            props.ChannelCount == curprofile.ChannelCount && props.SampleRate == curprofile.SampleRate &&
            props.Subtype == curprofile.Subtype);
    }
    public string DisplayValue
    {
      get
      {

        if (_props is VideoEncodingProperties)
        {
          var vprops = _props as VideoEncodingProperties;
          string subtype = null;
          if (vprops.Subtype.ToLowerInvariant() == "unknown" && vprops.Properties.ContainsKey(Guid.Parse("{f7e34c9a-42e8-4714-b74b-cb29d72c35e5}")))
          {
            subtype = vprops.Properties[Guid.Parse("{f7e34c9a-42e8-4714-b74b-cb29d72c35e5}")].ToString();
          }
          else
            subtype = vprops.Subtype;
          return string.Format("{0}, {1} x {2} , {3:##.#} fps , {4} {5}",
         subtype,
         vprops.Width, vprops.Height,
         vprops.FrameRate.Numerator / vprops.FrameRate.Denominator,
         vprops.Bitrate > (1024 * 1024) ? vprops.Bitrate / (1024 * 1024) : vprops.Bitrate / 1024, vprops.Bitrate > 1024 * 1024 ? "mbps" : "kbps");
        }
        else
        {
          var aprops = _props as AudioEncodingProperties;
          return string.Format("{0:##.#} {1} , {2} , {3} {4}",
          aprops.SampleRate > 1000 ? aprops.SampleRate / 1000 : aprops.SampleRate, aprops.SampleRate > 1000 ? "kHz" : "Hz",
          aprops.ChannelCount == 1 ? "Mono" : (aprops.ChannelCount == 2 ? "Stereo" : aprops.ChannelCount.ToString() + " channels"),
          aprops.Bitrate > 1024 * 1024 ? aprops.Bitrate / (1024 * 1024) : aprops.Bitrate / 1024, aprops.Bitrate > 1024 * 1024 ? "mbps" : "kbps");
        }

      }
    }


    public IMediaEncodingProperties Data
    {
      get
      {
        return _props;
      }

    }

    public DeviceWrapper DeviceInfo
    {
      get
      {
        return _devicewrapper;
      }


    }
  }

  public interface ISettingsPane
  {
    void OnClose();
  }

  [DataContract]
  public class IngestEndpointHistory
  {
    [DataMember]
    public string EndpointURL { get; set; }
    [DataMember]
    public long LastVideoTS { get; set; }
    [DataMember]
    public long LastAudioTS { get; set; }
    [DataMember]
    public string ServerType { get; set; }

  }

  [DataContract]
  public class StreamEndpointHistory
  {
    [DataMember]
    public string EndpointURL { get; set; }    
    [DataMember]
    public string StreamType { get; set; }

  }

  [DataContract]
  public class SessionHistory
  {

    public SessionHistory()
    {
      IngestHistory = new ObservableCollection<IngestEndpointHistory>();
      StreamPreviewURLHistory = new ObservableCollection<StreamEndpointHistory>();
    }
    [DataMember]
    public ObservableCollection<IngestEndpointHistory> IngestHistory { get; set; }

    [DataMember]
    public ObservableCollection<StreamEndpointHistory> StreamPreviewURLHistory { get; set; }
  }


}
