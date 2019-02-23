Public Class MainForm

    Private WithEvents mCamera As New ToFCamera()

    ' Type of image to display.

    Private Enum ImageType
        Intensity
        RangeMapColor
        RangeMapGrey
        Confidence
    End Enum

    ' Items to be put into the combo box.
    Private Class ComboBoxItem
        Public Sub New(value As ImageType, text As String)
            value = value
            text = text
        End Sub

        Public Property Text() As String

        Public Property Value() As ImageType

        Public Overrides Function ToString() As String
            Return Text
        End Function
    End Class

    ' The image type selected.
    Private mCurrentImageType As ImageType = ImageType.RangeMapColor

    Public Sub New()
        InitializeComponent()

        ' Intialize image type drop-down list.
        Dim comboBoxDataSource As New Dictionary(Of ImageType, String)()
        comboBoxDataSource.Add(ImageType.Confidence, "Confidence image")
        comboBoxDataSource.Add(ImageType.Intensity, "Intensity image")
        comboBoxDataSource.Add(ImageType.RangeMapColor, "Range map (color)")
        comboBoxDataSource.Add(ImageType.RangeMapGrey, "Range map (gray)")
        cboImageType.DataSource = New BindingSource(comboBoxDataSource, Nothing)
        cboImageType.ValueMember = "Key"
        cboImageType.DisplayMember = "Value"
        cboImageType.SelectedValue = ImageType.RangeMapColor

        ' Update the user interface elements.
        UpdateUI()

    End Sub

    Private Sub cboImageType_SelectedValueChanged(sender As Object, e As EventArgs) Handles cboImageType.SelectedValueChanged
        If cboImageType.SelectedIndex <> -1 Then
            ' Am item has been selected.
            ' Update the current image type according to the selection and
            ' reconfigure the camera to send the desired data type.
            Dim value As [Object] = cboImageType.SelectedValue
            If TypeOf value Is ImageType Then
                mCurrentImageType = DirectCast(value, ImageType)
                If mCamera.IsGrabbing() Then
                    Try
                        mCamera.StopGrabbing()
                        ConfigureCamera()
                        mCamera.StartGrabbing()
                    Catch exception As Exception
                        ShowException(exception)
                    End Try
                End If
            End If
        End If
    End Sub

    Private Sub btnClose_Click(sender As Object, e As EventArgs) Handles btnClose.Click
        Application.Exit()
    End Sub

    ' Open a camera if required, start grabbing.
    Private Sub btnStart_Click(sender As Object, e As EventArgs) Handles btnStart.Click
        Try
            ' Open the camera if it is not already opened.
            If Not mCamera.IsOpen() Then
                Cursor.Current = Cursors.WaitCursor
                ' We are going to use the first camera found.
                mCamera.OpenFirstCamera()
            End If

            ' Configure the component and the pixel format of the component that you want the camera to send.
            ConfigureCamera()

            ' Let the camera grab images continuously until either we call StopGrabbing or
            ' the GrabImageEvent handler signals to stop image acquisition.
            mCamera.StartGrabbing()
        Catch exception As Exception
            ShowException(exception)
        Finally
            Cursor.Current = Cursors.[Default]
        End Try
        ' Update state of the user interface elements.
        UpdateUI()

    End Sub

    ' Stop grabbing.
    Private Sub btnStop_Click(sender As Object, e As EventArgs) Handles btnStop.Click
        Try
            mCamera.StopGrabbing()
        Catch exception As Exception
            ShowException(exception)
        End Try
        ' Update state of the user interface elements.
        UpdateUI()
    End Sub

    ' Update state of the user interface elements.
    Private Sub UpdateUI()
        btnStart.Enabled = Not mCamera.IsGrabbing()
        btnStop.Enabled = mCamera.IsGrabbing()
    End Sub

    ' Display an exception in a message box.
    Private Sub ShowException(exception As Exception)
        MessageBox.Show(exception.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.[Error])
    End Sub


    ' Called when the application is about to be closed. Stop grabbing, close the camera 
    ' connection and free resources.
    Private Sub MainForm_FormClosing(sender As Object, ev As FormClosingEventArgs) Handles Me.FormClosing
        ' Close the camera connection. If there is a continuous grab operation, it will be stopped.
        If mCamera.IsOpen() Then
            mCamera.Close()
        End If
        ' Free all resources related to the camera device.
        mCamera.Dispose()
    End Sub


    ' Handles the ImageGrabbed event. The event args contain information about the
    ' data grabbed.
    ' Be aware that the image grabbed handler will be called from a thread that is not
    ' the UI thread. Therefore, operations that must be performed by the
    ' UI thread must be marshalled to the UI thread. The BeginInvoke function takes care
    ' of the marshalling.
    Private Sub ImageGrabbedHandler(sender As [Object], e As ImageGrabbedEventArgs) Handles mCamera.ImageGrabbed
        ' Check if the grab was successful.
        If e.status = GrabResultStatus.Ok Then
            ' Data was grabbed successfully. Now, the data can be processed.
            Dim part = e.parts(0)
            ' Convert the depth, intensity, or confidence data into a bitmap.
            Dim bitmap As Bitmap = ToFUtil.Converter.PartToBitmap(part)
            ' Let the picture box display the bitmap.
            BeginInvoke(Sub() pictureBox1.Image = bitmap)
        ElseIf e.status = GrabResultStatus.Timeout Then
            ' A timeout occurred. The timeout might be caused by the removal of the camera. Check if the camera
            ' is still connected.
            If Not DirectCast(sender, ToFCamera).IsConnected() Then
                ' Camera has been removed.
                BeginInvoke(Sub() MessageBox.Show("Camera has been removed!", "Error"))
            Else
                ' Camera is still connected. The reason for the timeout is unknown.
                BeginInvoke(Sub() MessageBox.Show("Timeout!", "Error"))
            End If
            ' If a timeout occurs, we want to stop grabbing.
            ' Indicate to stop image grabbing.
            e.stop = True
        Else
            ' Data wasn't grabbed successfully. Display an error message.
            BeginInvoke(Sub() MessageBox.Show("Grab error occurred!", "Error"))
        End If
    End Sub


    ' Configure the component type that you want the camera to send. Also configure
    ' the pixel format for the component enabled. 
    Private Sub ConfigureCamera()
        Try
            Dim enableRangeImage As Boolean = False
            Dim enableConfidenceImage As Boolean = False
            Dim enableIntensityImage As Boolean = False
            Dim PixelFormat As String = "Mono16"

            ' Decide which component to enable and which pixel format to select.
            Select Case mCurrentImageType
                ' Display the confidence map.
                Case ImageType.Confidence
                    enableConfidenceImage = True
                    PixelFormat = "Confidence16"
                    Exit Select
                    ' Display the intensity image.
                Case ImageType.Intensity
                    enableIntensityImage = True
                    PixelFormat = "Mono16"
                    Exit Select
                    ' Display a colored range map.
                Case ImageType.RangeMapColor
                    enableRangeImage = True
                    PixelFormat = "RGB8"
                    ' Depth information encoded as RGB colors.
                    Exit Select
                    ' Display a grey value range map.
                Case ImageType.RangeMapGrey
                    enableRangeImage = True
                    PixelFormat = "Coord3D_C16"
                    ' Depth information encoded as grey values.
                    Exit Select
            End Select

            ' Enable the component that contains the desired data. Disable all other
            ' components. Configure the pixel format for the component enabled.

            ' Set up the range map component.
            mCamera.SetParameterValue("ComponentSelector", "Range")
            mCamera.SetParameterValue("ComponentEnable", enableRangeImage.ToString().ToLower())
            If enableRangeImage Then
                mCamera.SetParameterValue("PixelFormat", PixelFormat)
            End If

            ' Set up the intensity image component.
            mCamera.SetParameterValue("ComponentSelector", "Intensity")
            mCamera.SetParameterValue("ComponentEnable", enableIntensityImage.ToString().ToLower())
            If enableIntensityImage Then
                mCamera.SetParameterValue("PixelFormat", PixelFormat)
            End If

            ' Set up the confidence map component.
            mCamera.SetParameterValue("ComponentSelector", "Confidence")
            mCamera.SetParameterValue("ComponentEnable", enableConfidenceImage.ToString().ToLower())
            If enableConfidenceImage Then
                mCamera.SetParameterValue("PixelFormat", PixelFormat)
            End If
        Catch exception As Exception
            ShowException(exception)
        End Try
    End Sub


End Class
