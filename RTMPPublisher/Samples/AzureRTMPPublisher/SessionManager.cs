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
using System.IO;
using System.Linq;
using System.Runtime.Serialization.Json;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;
using Windows.Media.Capture;
using Windows.Media.MediaProperties;
using Windows.Storage;
using Windows.UI.Xaml.Controls;
using Microsoft.Media.RTMP;

namespace RTMPPublisher
{
  public class SessionManager : INotifyPropertyChanged
  {
    bool _isPreviewing = false;
    public bool IsPreviewing
    {
      get
      {
        return _isPreviewing;
      }
      set
      {
        _isPreviewing = value;
      }
    }

    public List<DeviceWrapper> Cameras
    {
      get
      {
        return _cameras;
      }
    }


    public List<DeviceWrapper> Microphones
    {
      get
      {
        return _microphones;
      }
    }


    public MediaCapture CurrentCapture
    {
      get
      {
        return _capture;
      }
      set
      {
        _capture = value;
        if (PropertyChanged != null)
          PropertyChanged(this, new PropertyChangedEventArgs("CurrentCapture"));
      }
    }

    public ObservableCollection<PublishProfile> PublishProfiles
    {
      get
      {

        return _PublishProfiles;
      }

      set
      {
        _PublishProfiles = value;
      }
    }

    public bool UseContinuousTimestamps
    {
      get
      {
        return _useContinousTimestamps;
      }

      set
      {
        _useContinousTimestamps = value;
        if (PropertyChanged != null)
          PropertyChanged(this, new PropertyChangedEventArgs("UseContinuousTimestamps"));
      }
    }

    public bool EnableLowLatency
    {
      get
      {
        return _enableLowLatency;
      }

      set
      {
        _enableLowLatency = value;
        if (PropertyChanged != null)
          PropertyChanged(this, new PropertyChangedEventArgs("EnableLowLatency"));
      }
    }

    public bool IsPublishing
    {
      get
      {
        return _isPublishing;
      }

      set
      {
        _isPublishing = value;
        if (PropertyChanged != null)
          PropertyChanged(this, new PropertyChangedEventArgs("IsPublishing"));
      }
    }

    public SessionHistory History
    {
      get
      {
        return _history;
      }

      set
      {
        _history = value;
      }
    }

    public string StreamURL
    {
      get
      {
        return _streamURL;
      }

      set
      {
        _streamURL = value;
      }
    }

    public string StreamType
    {
      get
      {
        return _streamType;
      }

      set
      {
        _streamType = value;
      }
    }



    public SessionManager()
    {

    }

    public async Task InitializeAsync(CaptureElement ce)
    {

      _ceSourcePreview = ce;

      try
      {

        _cameras = (await DeviceInformation.FindAllAsync(DeviceClass.VideoCapture)).Select(di => new DeviceWrapper(di, DeviceClass.VideoCapture)).ToList();
        _microphones = (await DeviceInformation.FindAllAsync(DeviceClass.AudioCapture)).Select(di => new DeviceWrapper(di, DeviceClass.AudioCapture)).ToList();

        foreach (DeviceWrapper di in _cameras)
          await di.LoadEncodingPropertiesAsync();
        foreach (DeviceWrapper di in _microphones)
          await di.LoadEncodingPropertiesAsync();

      }
      catch
      {

      }
      return;
    }


    public async Task OnAppSuspending()
    {
      try
      {

        await SaveHistoryAsync();

        if (IsPublishing)
          await CurrentCapture.StopRecordAsync();

        IsPublishing = false;

      }
      catch (Exception ex)
      {

      }
    }


    public async Task RestartCapturePreviewAsync(string AudioDeviceID, string VideoDeviceID, AudioEncodingProperties audprops = null, VideoEncodingProperties vidprops = null)
    {
      if (_microphones.Count == 0 || _cameras.Count == 0)
        return;

      if (IsPreviewing)
      {
        await _capture.StopPreviewAsync();
      }

      try
      {
        if (_ceSourcePreview != null)
          _ceSourcePreview.Source = null;

        MediaCaptureInitializationSettings _initsettings = new MediaCaptureInitializationSettings();
        _initsettings.StreamingCaptureMode = StreamingCaptureMode.AudioAndVideo;
        _initsettings.AudioDeviceId = AudioDeviceID == null ? _microphones[0].Info.Id : AudioDeviceID;
        _initsettings.VideoDeviceId = VideoDeviceID == null ? _cameras[0].Info.Id : VideoDeviceID;

        _capture = new MediaCapture();

        await _capture.InitializeAsync(_initsettings);


        if (vidprops != null)
        {
          await _capture.VideoDeviceController.SetMediaStreamPropertiesAsync(MediaStreamType.VideoPreview, vidprops);
          await _capture.VideoDeviceController.SetMediaStreamPropertiesAsync(MediaStreamType.VideoRecord, vidprops);
        }

        if (_ceSourcePreview != null)
          _ceSourcePreview.Source = _capture;

        await _capture.StartPreviewAsync();
        IsPreviewing = true;

      }
      catch
      {

      }


    }


    public void AddPublishProfile(uint? width, uint? height, uint? kfinterval, uint? chunksize, uint? videobitrate, string profile, uint? audiobitrate, uint? audiosamplingfreqency, bool? stereo)
    {

      PublishProfile pubprof = new PublishProfile(RTMPServerType.Azure);
      pubprof.TargetEncodingProfile = MediaEncodingProfile.CreateMp4(VideoEncodingQuality.HD720p);

      pubprof.TargetEncodingProfile.Container = null;


      var srcvideoencodingprops = CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties;
      var srcaudioencodingprops = CurrentCapture.AudioDeviceController.GetMediaStreamProperties(MediaStreamType.Audio) as AudioEncodingProperties;

      if (width != null)
        pubprof.TargetEncodingProfile.Video.Width = width.Value;
      else
        pubprof.TargetEncodingProfile.Video.Width = srcvideoencodingprops.Width;
      if (height != null)
        pubprof.TargetEncodingProfile.Video.Height = height.Value;
      else
        pubprof.TargetEncodingProfile.Video.Height = srcvideoencodingprops.Height;
      if (videobitrate != null)
        pubprof.TargetEncodingProfile.Video.Bitrate = videobitrate.Value;
      else
        pubprof.TargetEncodingProfile.Video.Bitrate = srcvideoencodingprops.Bitrate;

      if (kfinterval != null)
        pubprof.KeyFrameInterval = kfinterval.Value;

      if (chunksize != null)
        pubprof.ClientChunkSize = chunksize.Value;

      if (profile != null)
      {
        pubprof.TargetEncodingProfile.Video.ProfileId = profile.ToLowerInvariant() == "baseline" ? H264ProfileIds.Baseline : (profile.ToLowerInvariant() == "main" ? H264ProfileIds.Main : H264ProfileIds.High);
      }

      pubprof.TargetEncodingProfile.Video.FrameRate.Numerator = srcvideoencodingprops.FrameRate.Numerator;
      pubprof.TargetEncodingProfile.Video.FrameRate.Denominator = srcvideoencodingprops.FrameRate.Denominator;

      if(audiobitrate != null)
        pubprof.TargetEncodingProfile.Audio.Bitrate = audiobitrate.Value;
      else
        pubprof.TargetEncodingProfile.Audio.Bitrate = srcaudioencodingprops.Bitrate;

      if (stereo != null)
        pubprof.TargetEncodingProfile.Audio.ChannelCount = stereo.Value ? 2U : 1U;
      else
        pubprof.TargetEncodingProfile.Audio.Bitrate = srcaudioencodingprops.ChannelCount;

      if (audiosamplingfreqency != null)
        pubprof.TargetEncodingProfile.Audio.SampleRate = audiosamplingfreqency.Value;
      else
        pubprof.TargetEncodingProfile.Audio.SampleRate = srcaudioencodingprops.SampleRate;
     


      _PublishProfiles.Add(pubprof);
    }



    public MediaEncodingProfile GetInputProfile()
    {

      return new MediaEncodingProfile()
      {
        Container = null,
        Video = CurrentCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoRecord) as VideoEncodingProperties,
        Audio = CurrentCapture.AudioDeviceController.GetMediaStreamProperties(MediaStreamType.Audio) as AudioEncodingProperties
      };
    }
    public async Task LoadHistoryAsync()
    {
      StorageFile historyFile = (await ApplicationData.Current.LocalFolder.TryGetItemAsync("SessionHistory.txt")) as StorageFile;
      if (historyFile == null)
      {
        historyFile = await ApplicationData.Current.LocalFolder.CreateFileAsync("SessionHistory.txt", CreationCollisionOption.ReplaceExisting);
      }


      var content = await FileIO.ReadTextAsync(historyFile);

      if (String.IsNullOrEmpty(content))
      {
        _history = new SessionHistory();
        return;
      }
      else
      {
        using (var s = (await historyFile.OpenReadAsync()).AsStreamForRead())
        {
          DataContractJsonSerializer jsonser = new DataContractJsonSerializer(typeof(SessionHistory));
          _history = jsonser.ReadObject(s) as SessionHistory;
        }
      }
    }

    public async Task SaveHistoryAsync()
    {
      StorageFile historyFile = await ApplicationData.Current.LocalFolder.CreateFileAsync("SessionHistory.txt", CreationCollisionOption.ReplaceExisting);

      DataContractJsonSerializer jsonser = new DataContractJsonSerializer(typeof(SessionHistory));

      using (var s = (await historyFile.OpenAsync(FileAccessMode.ReadWrite)).AsStream())
      {
        jsonser.WriteObject(s, _history);
        await s.FlushAsync();
      }
    }



    public IngestEndpointHistory AddIngestURLToHistory(string url, string serverType, long VideoTS = 0, long AudioTS = 0)
    {
      var match = _history.IngestHistory.Where(i => i.EndpointURL.ToLowerInvariant() == url.ToLowerInvariant() && i.ServerType == serverType).FirstOrDefault();
      if (match != null)
      {
        if (VideoTS != 0 && match.LastVideoTS != VideoTS)
          match.LastVideoTS = VideoTS;
        if (AudioTS != 0 && match.LastAudioTS != AudioTS)
          match.LastAudioTS = AudioTS;
        return match;
      }
      else
      {
        if (_history.IngestHistory.Count > 25)
          _history.IngestHistory.RemoveAt(0);

        var entry = new IngestEndpointHistory() { EndpointURL = url, ServerType = serverType, LastVideoTS = VideoTS, LastAudioTS = AudioTS };
        _history.IngestHistory.Add(entry);
        return entry;
      }

    }

    public StreamEndpointHistory AddStreamURLToHistory(string url, string streamType)
    {
      var match = _history.StreamPreviewURLHistory.Where(i => i.EndpointURL.ToLowerInvariant() == url.ToLowerInvariant() && i.StreamType.ToLowerInvariant() == streamType.ToLowerInvariant()).FirstOrDefault();
      if (match != null)
        return match;
      else
      {
        if (_history.StreamPreviewURLHistory.Count > 25)
          _history.StreamPreviewURLHistory.RemoveAt(0);

        var entry = new StreamEndpointHistory() { EndpointURL = url, StreamType = streamType };
        _history.StreamPreviewURLHistory.Add(entry);
        return entry;
      }

    }

    

    #region PRIVATE

    private List<DeviceWrapper> _cameras;
    private List<DeviceWrapper> _microphones;
    private MediaCapture _capture;
    private CaptureElement _ceSourcePreview;
    ObservableCollection<PublishProfile> _PublishProfiles = new ObservableCollection<PublishProfile>();

    bool _enableLowLatency = false;
    bool _useContinousTimestamps = false;
    bool _isPublishing = false;
    SessionHistory _history = null;
    string _streamURL = null;
    string _streamType = null;
    public event PropertyChangedEventHandler PropertyChanged;

    #endregion
  }


}
