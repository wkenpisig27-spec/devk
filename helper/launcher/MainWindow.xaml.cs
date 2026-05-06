using System;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace SlimePiratesLauncher
{
    public partial class MainWindow : Window
    {
        private UpdaterLogic _updater;

        public MainWindow()
        {
            InitializeComponent();
            _updater = new UpdaterLogic(this);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            // Load background image
            string imagePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "updater", "assets", "bg_img.png");
            if (File.Exists(imagePath))
            {
                bgImage.Source = new System.Windows.Media.Imaging.BitmapImage(new Uri(imagePath, UriKind.Absolute));
            }

            // Check if repair mode should start automatically
            string repairPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "Updater", "repair.dat");
            if (File.Exists(repairPath))
            {
                _updater.StartRepair();
            }
            else
            {
                // Start normal update process
                _updater.StartUpdate();
            }
        }

        // --- Button Handlers ---

        private void BtnPlay_Click(object sender, RoutedEventArgs e)
        {
            try 
            {
                string gamePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "system", "Game.exe");
                if (File.Exists(gamePath))
                {
                    Process.Start(gamePath, "7k9mX2pQ4wR8nL3vZ6hT");
                    Application.Current.Shutdown();
                }
                else
                {
                    MessageBox.Show("Game.exe not found in system folder!", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Could not start Game.exe!\n" + ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void BtnWebsite_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string url = !string.IsNullOrEmpty(_updater.WebUrl) ? _updater.WebUrl : "https://example.com";
                Process.Start(new ProcessStartInfo(url) { UseShellExecute = true });
            }
            catch (Exception ex)
            {
                MessageBox.Show("Could not open website!\n" + ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void BtnCheck_Click(object sender, RoutedEventArgs e)
        {
            _updater.StartCheck();
        }

        // --- Bridge Methods for Updater ---

        public void SetStatus(string text)
        {
            Dispatcher.Invoke(() => txtStatus.Text = text);
        }

        public void SetProgress(int value, int max)
        {
             Dispatcher.Invoke(() => {
                progressBar.Maximum = max;
                progressBar.Value = value;
            });
        }

        public void EnablePlay(bool enable)
        {
             Dispatcher.Invoke(() => btnPlay.IsEnabled = enable);
        }

        public void SetStatusColor(Color color)
        {
            Dispatcher.Invoke(() => {
                txtStatus.Foreground = new SolidColorBrush(color);
            });
        }
    }
}
