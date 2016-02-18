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

using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Linq;
using Windows.Storage;
using Windows.UI.Core;
using Windows.Web.Http;
using Windows.Web.Http.Filters;

namespace RTMPPublisher
{
  [JsonObject]
  public class ACSTokenResponse
  {
    [JsonProperty(PropertyName = "token_type")]
    public string TokenType { get; set; }
    [JsonProperty(PropertyName = "access_token")]
    public string AccessToken { get; set; }
    [JsonProperty(PropertyName = "expires_in")]
    public uint ExpiresIn { get; set; }
    [JsonProperty(PropertyName = "scope")]
    public string Scope { get; set; }
  }

  [JsonObject]
  public class InputEndpoint : INotifyPropertyChanged
  {
    [JsonProperty(PropertyName = "Protocol")]
    public string Protocol { get; set; }
    [JsonProperty(PropertyName = "Url")]
    public string Url { get; set; }

    public event PropertyChangedEventHandler PropertyChanged;
  }

  [JsonObject]
  public class PreviewEndpoint : INotifyPropertyChanged
  {
    [JsonProperty(PropertyName = "Protocol")]
    public string Protocol { get; set; }
    [JsonProperty(PropertyName = "Url")]
    public string Url { get; set; }

    private bool _isStreaming = false;

    public bool IsStreaming
    {
      get { return _isStreaming; }
      set
      {
        var oldval = _isStreaming;
        _isStreaming = value;


        if (PropertyChanged != null)
          PropertyChanged(this, new PropertyChangedEventArgs("IsStreaming"));

      }
    }


    public event PropertyChangedEventHandler PropertyChanged;
  }

  [JsonObject]
  public class ChannelInput : INotifyPropertyChanged
  {
    [JsonProperty(PropertyName = "Endpoints")]
    public List<InputEndpoint> Endpoints { get; set; }

    public event PropertyChangedEventHandler PropertyChanged;
  }

  [JsonObject]
  public class ChannelPreview : INotifyPropertyChanged
  {

    private ObservableCollection<PreviewEndpoint> _endpoints = null;
    [JsonProperty(PropertyName = "Endpoints")]
    public ObservableCollection<PreviewEndpoint> Endpoints
    {
      get
      {
        return _endpoints;
      }
      set
      {
        _endpoints = value;

      }
    }

    private bool _isStreaming = false;

    public bool IsStreaming
    {
      get { return _isStreaming; }
      set
      {
        var oldval = _isStreaming;
        _isStreaming = value;

        if (PropertyChanged != null && oldval != value)
        {
          PropertyChanged(this, new PropertyChangedEventArgs("IsStreaming"));
          PropertyChanged(this, new PropertyChangedEventArgs("SmoothUri"));
        }

      }
    }

    public Uri SmoothUri
    {
      get
      {
        var pep = Endpoints.Where(p => p.Protocol == "FragmentedMP4").FirstOrDefault();
        if (pep != null)
          return new Uri(pep.Url);
        else
          return null;
      }
    }

    public event PropertyChangedEventHandler PropertyChanged;
  }

  [JsonObject]
  public class Creds
  {
    [JsonProperty("AccountName")]
    public string AccountName { get; set; }

    [JsonProperty("AccountKey")]
    public string AccountKey { get; set; }
  }

  [JsonObject]
  public class Channel : INotifyPropertyChanged
  {
    [JsonProperty(PropertyName = "Id")]
    public string Id { get; set; }

    [JsonProperty(PropertyName = "Name")]
    public string Name { get; set; }


    [JsonProperty(PropertyName = "Description")]
    public string Description { get; set; }

    private string _state = null;
    [JsonProperty(PropertyName = "State")]
    public string State
    {
      get
      {
        return _state;
      }
      set
      {
        var oldval = _state;
        _state = value;
        if (_dispatcher != null)
        {
          _dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
           {
             if (oldval != value && PropertyChanged != null)
               PropertyChanged(this, new PropertyChangedEventArgs("State"));
           });
        }
        else
        {
          if (oldval != value && PropertyChanged != null)
            PropertyChanged(this, new PropertyChangedEventArgs("State"));
        }
      }
    }


    [JsonProperty(PropertyName = "Input")]
    public ChannelInput Input { get; set; }

    [JsonProperty(PropertyName = "Preview")]
    public ChannelPreview Preview { get; set; }
 
    public bool UseMonotonicTS { get; set; } 

    public event PropertyChangedEventHandler PropertyChanged;

    public string ResetOperationID = null;
    public Timer ResetCheckTimer;
    CoreDispatcher _dispatcher = null;

    public void StartResetCheck(TimerCallback callback,string ResetOpID,CoreDispatcher disp)
    {
      _dispatcher = disp;
      ResetOperationID = ResetOpID;
      ResetCheckTimer = new Timer(callback, this, 1000, 8000);
    }

    public void StopResetCheck()
    {
      ResetCheckTimer.Change(Timeout.Infinite, Timeout.Infinite);
     
    }
  
  }
  public class AMSRESTWrapper
  {

    private Creds creds = null;
    
    string _acsUri = "https://wamsprodglobal001acs.accesscontrol.windows.net/v2/OAuth2-13";
    Uri _mediaServicesUri = new Uri("https://media.windows.net/");
    ACSTokenResponse _acsTokenInfo = null;

    public Creds Credentials
    {
      get
      {
        return creds;
      }

      set
      {
        creds = value;
      }
    }

    public AMSRESTWrapper()
    {
    }


    public async void SaveCredentialsAsync(string accountname, string key)
    {
      StorageFile credsFile = await ApplicationData.Current.LocalFolder.CreateFileAsync("Creds.txt", CreationCollisionOption.ReplaceExisting);

      var payload = JsonConvert.SerializeObject(new Creds() { AccountName = accountname, AccountKey = key });
    

      using (var s = (await credsFile.OpenAsync(FileAccessMode.ReadWrite)).AsStream())
      {
        using (StreamWriter tw = new StreamWriter(s))
        {
          tw.Write(payload);
          await tw.FlushAsync();
        }
      }

       
    }

    public async Task<bool> LoadCredentialsAsync()
    {
      try
      {
        StorageFile credsFile = (await ApplicationData.Current.LocalFolder.TryGetItemAsync("Creds.txt")) as StorageFile;
        if (credsFile == null)
        {
          return false;
        }


        var content = await FileIO.ReadTextAsync(credsFile);

        if (String.IsNullOrEmpty(content))
        {

          return false;
        }
        else
        {
          using (var s = (await credsFile.OpenReadAsync()).AsStreamForRead())
          {
            StreamReader br = new StreamReader(s);
            var payload = br.ReadToEnd();
            if (payload == null || payload.Trim() == "")
              return false;
            this.creds = JsonConvert.DeserializeObject<Creds>(payload);
            return true;
          }
        }
      }
      catch
      {
       
      }

      creds = null;
      return false;
    }
    public async Task<bool> AuthorizeAsync()
    {
      try
      {

        HttpClient httpClient = new HttpClient();
        HttpRequestMessage reqMessage = new HttpRequestMessage(HttpMethod.Post, new Uri(_acsUri))
        {
          Content = //new HttpStringContent(string.Format("grant_type=client_credential&client_id={0}&client_secret={1}&scope=urn%3aWindowsAzureMediaServices",_accountName,_accountKey))

        new HttpFormUrlEncodedContent(new Dictionary<string, string>() {
      { "grant_type","client_credentials"},
      {"client_id",Credentials.AccountName },
      { "client_secret",Credentials.AccountKey },
      { "scope","urn:WindowsAzureMediaServices" } })
        };

        //if(reqMessage.Headers.ContainsKey("Content-Type"))
        //  reqMessage.Headers["Content-Type"] = "application/x-www-form-urlencoded";
        //else
        //  reqMessage.Headers.Add("Content-Type", "application/x-www-form-urlencoded");
        if (reqMessage.Headers.ContainsKey("Accept"))
          reqMessage.Headers["Accept"] = "application/json";
        else
          reqMessage.Headers.Add("Accept", "application/json");
        if (reqMessage.Headers.ContainsKey("Expect"))
          reqMessage.Headers["Expect"] = "100-continue";
        else
          reqMessage.Headers.Add("Expect", "100-continue");
        if (reqMessage.Headers.ContainsKey("Connection"))
          reqMessage.Headers["Connection"] = "Keep-Alive";
        else
          reqMessage.Headers.Add("Connection", "Keep-Alive");

        var respMessage = await httpClient.SendRequestAsync(reqMessage);

        if (!respMessage.IsSuccessStatusCode)
          return false;

        _acsTokenInfo = JsonConvert.DeserializeObject<ACSTokenResponse>(await respMessage.Content.ReadAsStringAsync());

        if (_acsTokenInfo == null)
          return false;
      }
      catch
      {
        return false;
      }

      return await ValidateMediaServiceEndpoint();
    }

    private async Task<bool> ValidateMediaServiceEndpoint()
    {
      if (_acsTokenInfo == null)
        return false;

      HttpResponseMessage respMessage = null;
      try
      {
        var filter = new HttpBaseProtocolFilter() { AllowAutoRedirect = false };
        HttpClient httpClient = new HttpClient(filter);
        HttpRequestMessage reqMessage = new HttpRequestMessage(HttpMethod.Get, _mediaServicesUri);
        AddAMSHeaders(reqMessage);

        respMessage = await httpClient.SendRequestAsync(reqMessage);

        if (respMessage.StatusCode == Windows.Web.Http.HttpStatusCode.MovedPermanently) //redirect
        {
          _mediaServicesUri = respMessage.Headers.Location;

          HttpClient httpClient2 = new HttpClient();
          reqMessage = new HttpRequestMessage(HttpMethod.Get, _mediaServicesUri);
          AddAMSHeaders(reqMessage);

          respMessage = await httpClient.SendRequestAsync(reqMessage);

        }

        if (!respMessage.IsSuccessStatusCode)
        {
          return false;
        }


      }
      catch
      {
        return false;
      }

      return true;
    }


    public async Task GetChannelsAsync(ObservableCollection<Channel> channelList)
    {
      if (_acsTokenInfo == null)
        return;

      Uri channelListUri = null;
      Uri.TryCreate(_mediaServicesUri, "Channels", out channelListUri);


      try
      {

        HttpClient httpClient = new HttpClient();

        HttpRequestMessage reqMessage = new HttpRequestMessage(HttpMethod.Get, channelListUri);
        AddAMSHeaders(reqMessage);

        var respMessage = await httpClient.SendRequestAsync(reqMessage);

        if (!respMessage.IsSuccessStatusCode)
        {
          return;
        }
        else
        {
          var retval = JObject.Parse(await respMessage.Content.ReadAsStringAsync())["value"]
            .Children()
            .Select((jt) =>
            {
             return JsonConvert.DeserializeObject<Channel>(jt.ToString()); 
            }).ToList();

          foreach (Channel c in retval)
          {
            await CheckChannelStatusAsync(c,true);
            channelList.Add(c);
          }
        }
      }
      catch
      {
        return;
      }

      return;
    }

    
    public async Task CheckChannelStatusAsync(Channel chnl, bool SkipStateCheck = false)
    {

      if (!SkipStateCheck)
      {

        if (_acsTokenInfo == null)
          return;

        Uri channelUri = null;
        Uri.TryCreate(_mediaServicesUri, string.Format("Channels('{0}')", chnl.Id), out channelUri);


        try
        {

          HttpClient httpClient = new HttpClient();

          HttpRequestMessage reqMessage = new HttpRequestMessage(HttpMethod.Get, channelUri);
          AddAMSHeaders(reqMessage);

          var respMessage = await httpClient.SendRequestAsync(reqMessage);

          if (!respMessage.IsSuccessStatusCode)
          {
            return;
          }
          else
          {
            var retval = JsonConvert.DeserializeObject<Channel>(await respMessage.Content.ReadAsStringAsync());
            if(chnl.State != "Resetting")
              chnl.State = retval.State;
          }
        }
        catch
        {
          return;
        }
      } 

      foreach (PreviewEndpoint ep in chnl.Preview.Endpoints)
      {
        try
        {

          HttpClient httpClient = new HttpClient();
          HttpRequestMessage reqMessage = new HttpRequestMessage(HttpMethod.Get, new Uri(ep.Url));

          var respMessage = await httpClient.SendRequestAsync(reqMessage);

          if (!respMessage.IsSuccessStatusCode)
            ep.IsStreaming = false;
          else
          {
          
            if (ep.Protocol == "FragmentedMP4")
            {
              var ct = System.IO.WindowsRuntimeStreamExtensions.AsStreamForRead(await respMessage.Content.ReadAsInputStreamAsync());
              XDocument xDoc = XDocument.Load(ct);
              var streamindices = xDoc.Descendants().Where(xe => xe.Name == "StreamIndex").ToList();
              foreach (XElement xe in streamindices)
              {
                if (xe.Nodes().Count(xn => xn is XElement && (xn as XElement).Name == "c") > 0)
                {
                  ep.IsStreaming = true;
                  chnl.Preview.IsStreaming = true;
                }
                else
                {
                  ep.IsStreaming = false;
                  chnl.Preview.IsStreaming = false;
                  break;
                }
              }
            }
            else
            {
              ep.IsStreaming = true;
              chnl.Preview.IsStreaming = true;
            }
          }

        }
        catch
        {
          ep.IsStreaming = false;
          chnl.Preview.IsStreaming = false;
        }
      }
    }

    public async Task<bool> ResetAsync(Channel c,CoreDispatcher disp)
    {
      if (_acsTokenInfo == null)
        return false;

      Uri channelResetUri = null;
      Uri.TryCreate(_mediaServicesUri, string.Format("Channels('{0}')/Reset",c.Id), out channelResetUri); 

      try
      {

        HttpClient httpClient = new HttpClient();

        HttpRequestMessage reqMessage = new HttpRequestMessage(HttpMethod.Post, channelResetUri);
        AddAMSHeaders(reqMessage);

        var respMessage = await httpClient.SendRequestAsync(reqMessage);

        if (respMessage.StatusCode == Windows.Web.Http.HttpStatusCode.Accepted && respMessage.Headers.ContainsKey("operation-id"))
        {
          c.State = "Resetting";
          
          c.StartResetCheck(new TimerCallback(ResetCheck),respMessage.Headers["operation-id"],disp);
          return true;
        }
        
      }
      catch
      {
        return false;
      }

      return false;
    }

    private async void ResetCheck(object c)
    {
      if (_acsTokenInfo == null)
        return ;
      Channel chnl = c as Channel;
      Uri channelResetOpUri = null;
      Uri.TryCreate(_mediaServicesUri, string.Format("Operations('{0}')", chnl.ResetOperationID), out channelResetOpUri);

      try
      {

        HttpClient httpClient = new HttpClient();

        HttpRequestMessage reqMessage = new HttpRequestMessage(HttpMethod.Get, channelResetOpUri);
        AddAMSHeaders(reqMessage);

        var respMessage = await httpClient.SendRequestAsync(reqMessage);

        if (respMessage.IsSuccessStatusCode)
        {
          var state = JObject.Parse(await respMessage.Content.ReadAsStringAsync())["State"].ToString();
          if (state.ToLowerInvariant() == "succeeded" || state.ToLowerInvariant() == "failed")
          {
            chnl.StopResetCheck();
            chnl.State = "Running";
          }
          return ;
        }

      }
      catch
      {
        return ;
      }

      return ;
    }
    private void AddAMSHeaders(HttpRequestMessage reqMessage)
    {
      reqMessage.Headers.Authorization = new Windows.Web.Http.Headers.HttpCredentialsHeaderValue("Bearer", _acsTokenInfo.AccessToken);
      reqMessage.Headers.Add("x-ms-version", "2.11");
      reqMessage.Headers.Add("DataServiceVersion", "3.0");
      reqMessage.Headers.Add("MaxDataServiceVersion", "3.0");
      reqMessage.Headers.Add("Accept", "application/json"); 
    }

  }
}
