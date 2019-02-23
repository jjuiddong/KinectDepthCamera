using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using ToFCameraWrapper;

namespace RangeMapCSharp
{
    public partial class MainForm : Form
    {
        private ToFCamera   mCamera = new ToFCamera();

        // Type of image to display.
        private enum ImageType {
            Intensity,
            RangeMapColor ,
            RangeMapGrey,
            Confidence
        };

        // Items to be put into the combo box.
        private class ComboBoxItem
        {
            public ComboBoxItem(ImageType value, string text)
            {
                Value = value;
                Text = text;
            }

            public string Text { get; set; }
            public ImageType Value { get; set; }

            public override string ToString()
            {
                return Text;
            }
        }

        // The image type selected.
        private ImageType mCurrentImageType = ImageType.RangeMapColor;


        public MainForm()
        {
            InitializeComponent();

            // Intialize image type drop-down list.
            Dictionary<ImageType, string> comboBoxDataSource = new Dictionary<ImageType, string>();
            comboBoxDataSource.Add(ImageType.Confidence, "Confidence map");
            comboBoxDataSource.Add(ImageType.Intensity, "Intensity image");
            comboBoxDataSource.Add(ImageType.RangeMapColor, "Range map (color)");
            comboBoxDataSource.Add(ImageType.RangeMapGrey, "Range map (gray)");
            cboImageType.DataSource = new BindingSource(comboBoxDataSource, null);
            cboImageType.ValueMember = "Key";
            cboImageType.DisplayMember = "Value";
            cboImageType.SelectedValueChanged += cboImageType_SelectedValueChanged;
            cboImageType.SelectedValue = ImageType.RangeMapColor;

            // Update the user interface elements.
            UpdateUI();
        }
     

        private void btnClose_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        // Open a camera if required, start grabbing.
        private void btnStart_Click(object sender, EventArgs e)
        {
            try
            {
                // Open the camera if it is not already opened.
                if (!mCamera.IsOpen())
                {
                    Cursor.Current = Cursors.WaitCursor;
                    // We are going to use the first camera found.
                    mCamera.OpenFirstCamera();

                    // Subscribe to the ImageEvent that will be raised for each image grabbed.
                    mCamera.ImageGrabbed += ImageGrabbedHandler;
                }
                // Configure the component and the pixel format of the component that you want the camera to send.
                ConfigureCamera();

                // Let the camera grab images continuously until either we call StopGrabbing or
                // the GrabImageEvent handler signals to stop image acquisition.
                mCamera.StartGrabbing();
            }
            catch (Exception exception)
            {
                ShowException(exception);
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }
            // Update state of the user interface elements.
            UpdateUI();
        }

        // Stop grabbing.
        private void btnStop_Click(object sender, EventArgs e)
        {
            try
            {
                mCamera.StopGrabbing();
            }
            catch (Exception exception)
            {
                ShowException(exception);
            }
            // Update state of the user interface elements.
            UpdateUI();
        }

        // Update state of the user interface elements.
        private void UpdateUI()
        {
            btnStart.Enabled = ! mCamera.IsGrabbing();
            btnStop.Enabled = mCamera.IsGrabbing();
        }

        // Display an exception in a message box.
        private void ShowException(Exception exception)
        {
            MessageBox.Show(exception.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        // Called when the application is about to be closed. Stop grabbing, close the camera 
        // connection and free resources.
        private void MainForm_FormClosing(object sender, FormClosingEventArgs ev)
        {
            // Close the camera connection. If there is a continuous grab operation, it will be stopped.
            if (mCamera.IsOpen())
            {
                mCamera.Close();
            }
            // Free all resources related to the camera device.
            mCamera.Dispose();
        }

        // Handles the ImageGrabbed event. The event args contain information about the
        // data grabbed.
        // Be aware that the image grabbed handler will be called from a thread that is not
        // the UI thread. Therefore, operations that must be performed by the
        // UI thread must be marshalled to the UI thread. The BeginInvoke function takes care
        // of the marshalling.
        private void ImageGrabbedHandler(Object sender, ImageGrabbedEventArgs e)
        {
            // Check if the grab was successful.
            if ( e.status == GrabResultStatus.Ok)
            {
                // Data was grabbed successfully. Now, the data can be processed.
                var part = e.parts[0];
                // Convert the depth, intensity, or confidence data into a bitmap.
                Bitmap bitmap = ToFUtil.Converter.PartToBitmap(part);
                // Let the picture box display the bitmap.
                BeginInvoke((Action) (() => pictureBox1.Image = bitmap));
            }
            else if ( e.status == GrabResultStatus.Timeout )
            {
                // A timeout occurred. The timeout might be caused by the removal of the camera. Check if the camera
                // is still connected.
                if (! ((ToFCamera) sender).IsConnected() )
                {
                    // Camera has been removed.
                    BeginInvoke( (Action) (() => MessageBox.Show("Camera has been removed!", "Error") ));
                }
                else
                {
                    // Camera is still connected. The reason for the timeout is unknown.
                    BeginInvoke( (Action) (() => MessageBox.Show("Timeout!", "Error" )) );
                }
                // If a timeout occurs, we want to stop grabbing.
                e.stop = true; // Indicate to stop image grabbing.
            }
            else
            {
                // Data wasn't grabbed successfully. Display an error message.
                BeginInvoke((Action)(() => MessageBox.Show("Grab error occurred!", "Error")));
            }
        }

        // Called when the user selects an item in the combo box.
        private void cboImageType_SelectedValueChanged(object sender, EventArgs e)
        {
            if (cboImageType.SelectedIndex != -1)
            {
                // Am item has been selected.
                // Update the current image type according to the selection and
                // reconfigure the camera to send the desired data type.
                Object value = cboImageType.SelectedValue;
                if (value is ImageType)
                {
                    mCurrentImageType = (ImageType)value;
                    if (mCamera.IsGrabbing())
                    {
                        try
                        {
                            mCamera.StopGrabbing();
                            ConfigureCamera();
                            mCamera.StartGrabbing();
                        }
                        catch ( Exception exception )
                        {
                            ShowException(exception);
                        }
                    }
                }
            }
        }

        // Configure the component type that you want the camera to send. Also configure
        // the pixel format for the component enabled. 
        private void ConfigureCamera()
        {
            try
            {
                bool enableRangeImage = false;
                bool enableConfidenceImage = false;
                bool enableIntensityImage = false;
                string PixelFormat = "Mono16";

                // Decide which component to enable and which pixel format to select.
                switch (mCurrentImageType)
                {
                    // Display the confidence map.
                    case ImageType.Confidence:
                        enableConfidenceImage = true;
                        PixelFormat = "Confidence16";
                        break;
                    // Display the intensity image.
                    case ImageType.Intensity:
                        enableIntensityImage = true;
                        PixelFormat = "Mono16";
                        break;
                    // Display a colored range map.
                    case ImageType.RangeMapColor:
                        enableRangeImage = true;
                        PixelFormat = "RGB8";  // Depth information encoded as RGB colors.
                        break;
                    // Display a grey value range map.
                    case ImageType.RangeMapGrey:
                        enableRangeImage = true;
                        PixelFormat = "Coord3D_C16";  // Depth information encoded as grey values.
                        break;
                }

                // Enable the component that contains the desired data. Disable all other
                // components. Configure the pixel format for the component enabled.

                // Set up the range map component.
                mCamera.SetParameterValue("ComponentSelector", "Range");
                mCamera.SetParameterValue("ComponentEnable", enableRangeImage.ToString().ToLower());
                if (enableRangeImage)
                {
                    mCamera.SetParameterValue("PixelFormat", PixelFormat);
                }

                // Set up the intensity image component.
                mCamera.SetParameterValue("ComponentSelector", "Intensity");
                mCamera.SetParameterValue("ComponentEnable", enableIntensityImage.ToString().ToLower());
                if (enableIntensityImage)
                {
                    mCamera.SetParameterValue("PixelFormat", PixelFormat);
                }

                // Set up the confidence map component.
                mCamera.SetParameterValue("ComponentSelector", "Confidence");
                mCamera.SetParameterValue("ComponentEnable", enableConfidenceImage.ToString().ToLower());
                if (enableConfidenceImage)
                {
                    mCamera.SetParameterValue("PixelFormat", PixelFormat);
                }
            }
            catch (Exception exception)
            {
                ShowException(exception);
            }
        }

    }
}
