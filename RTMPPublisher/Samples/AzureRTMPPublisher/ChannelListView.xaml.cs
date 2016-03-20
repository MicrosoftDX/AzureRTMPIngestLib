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
using System.Collections.ObjectModel;
using System.Threading;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Media;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

namespace RTMPPublisher
{

  public delegate void ChannelSelectHandler(Channel c);
  public sealed partial class ChannelListView : Page
  {

    ObservableCollection<Channel> _channels = new ObservableCollection<Channel>();
    AMSRESTWrapper _amswrapper = new AMSRESTWrapper();

    Timer _refreshTimer = null;
    public event ChannelSelectHandler Publish;
    public event ChannelSelectHandler Playback;

    public ChannelListView()
    {
      this.InitializeComponent();

      Loaded += ChannelListView_Loaded;
    }

    private void _refreshTimer_Tick(object state)
    {
      this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
       {
         foreach (var chnl in _channels)
           _amswrapper.CheckChannelStatusAsync(chnl);
       });

    }

    private async void ChannelListView_Loaded(object sender, RoutedEventArgs e)
    {
      prProgress.IsActive = true;
      gridInitialLoad.Visibility = Visibility.Visible;

      await _amswrapper.LoadCredentialsAsync();

      if (_amswrapper.Credentials == null || !(await LoadChannelListAsync()))
        gridCreds.Visibility = Visibility.Visible;

    }

    private async System.Threading.Tasks.Task<bool> LoadChannelListAsync()
    {
      prProgress.IsActive = true;
      gridLoading.Visibility = Visibility.Visible;
      bool auth = await _amswrapper.AuthorizeAsync();
      if (!auth)
      {

        gridLoading.Visibility = Visibility.Collapsed;
        return false;
      }

      _channels.Clear();
      await _amswrapper.GetChannelsAsync(_channels);

      TrackRefresh(true);

      lvChannels.ItemsSource = _channels;

      prProgress.IsActive = false;
      gridInitialLoad.Visibility = Visibility.Collapsed;
      gridChannels.Visibility = Visibility.Visible;

      return true;
    }

    private async void lvChannels_ItemClick(object sender, ItemClickEventArgs e)
    {

      Channel c = e.ClickedItem as Channel;
      if (c.State.ToLowerInvariant() != "running") return;

      TrackRefresh(false);


      await _amswrapper.CheckChannelStatusAsync(c, true);
      this.Frame.Navigate(typeof(PublishView), c);

    }

    public void TrackRefresh(bool State)
    {
      if (State)
      {
        _refreshTimer = new Timer(new TimerCallback(this._refreshTimer_Tick), null, 1000, 8000);
      }
      else
      {
        _refreshTimer.Change(Timeout.Infinite, Timeout.Infinite);
        _refreshTimer = null;
      }
    }

    private async void btnReset_Click(object sender, RoutedEventArgs e)
    {
      Channel c = (sender as Button).DataContext as Channel;
      (Application.Current as App).DeviceManager.AddIngestURLToHistory(c.Input.Endpoints[0].Url, "AZURE", 0, 0);
      await _amswrapper.ResetAsync(c, this.Dispatcher);
      return;
    }

    private async void btnLoad_Click(object sender, RoutedEventArgs e)
    {
      gridCreds.Visibility = Visibility.Collapsed;
      _amswrapper.Credentials = new Creds() { AccountName = tbxacctname.Text, AccountKey = tbxacctkey.Text };
      if (await LoadChannelListAsync())
      {
        _amswrapper.SaveCredentialsAsync(_amswrapper.Credentials.AccountName, _amswrapper.Credentials.AccountKey);
      }
      else
      {
        gridCreds.Visibility = Visibility.Visible;
      }
    }

    private void btnChangeCreds_Click(object sender, RoutedEventArgs e)
    {
      gridLoading.Visibility = Visibility.Collapsed;
      gridChannels.Visibility = Visibility.Collapsed;
      gridInitialLoad.Visibility = Visibility.Visible;
      gridCreds.Visibility = Visibility.Visible;
      if (_amswrapper.Credentials != null)
      {
        tbxacctname.Text = _amswrapper.Credentials.AccountName;
        tbxacctkey.Text = _amswrapper.Credentials.AccountKey;
      }

    }
  }

  public class ChannelStreamingStateToColorConverter : IValueConverter
  {
    public object Convert(object value, Type targetType, object parameter, string language)
    {
      if ((bool)value == true)
        return new SolidColorBrush(Colors.Green);
      else
        return new SolidColorBrush(Colors.Gray);
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
      throw new NotImplementedException();
    }
  }
  public class ChannelStateToColorConverter : IValueConverter
  {
    public object Convert(object value, Type targetType, object parameter, string language)
    {
      if (((string)value).ToLowerInvariant() == "running")
        return new SolidColorBrush(Colors.Green);
      else
        return new SolidColorBrush(Colors.Black);
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
      throw new NotImplementedException();
    }
  }

  public class ChannelStateToBoolConverter : IValueConverter
  {
    public object Convert(object value, Type targetType, object parameter, string language)
    {
      if (((string)value).ToLowerInvariant() == "resetting")
        return false;
      else
        return true;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
      throw new NotImplementedException();
    }
  }
  public class ChannelStateToVisibilityConverter : IValueConverter
  {
    public object Convert(object value, Type targetType, object parameter, string language)
    {
      if (((string)value).ToLowerInvariant() == "resetting")
        return Visibility.Visible;
      else
        return Visibility.Collapsed;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
      throw new NotImplementedException();
    }
  }



  public class BoolToVisibilityConverter : IValueConverter
  {
    public object Convert(object value, Type targetType, object parameter, string language)
    {
      string p = parameter as string;
      if (p != null && p == "reverse")
        return (bool)value ? Visibility.Collapsed : Visibility.Visible;
      else
        return (bool)value ? Visibility.Visible : Visibility.Collapsed;

    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
      throw new NotImplementedException();
    }
  }
}
