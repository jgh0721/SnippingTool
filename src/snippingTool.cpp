#include "snippingTool.hpp"
#include "dxgiMgr.hpp"

///////////////////////////////////////////////////////////////////////////////
///
///

QSnippingWidget::QSnippingWidget( QPixmap Scr )
    : screenShot_( Scr ), isSelecting_( false )
{
    setCursor( Qt::CrossCursor );
    setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint );
    showFullScreen();
    setWindowState( Qt::WindowFullScreen );
}

QPixmap QSnippingWidget::SelectedRegion()
{
    return selectedRegion_;
}

void QSnippingWidget::SetDisplayAffinity( quint32 dwAffinity )
{
    dwAffinity_ = dwAffinity;
    ::SetWindowDisplayAffinity( (HWND)winId(), dwAffinity_ );
}

void QSnippingWidget::paintEvent( QPaintEvent* event )
{
    ::SetWindowDisplayAffinity( (HWND)winId(), dwAffinity_ );

    QPainter painter( this );
    painter.save();
    painter.scale( 1.0 / devicePixelRatio(), 1.0 / devicePixelRatio() );
    painter.drawPixmap( 0, 0, screenShot_ );
    painter.restore();

    QColor overlayColor( 0, 0, 0, 120 );
    painter.fillRect( QRect( 0, 0, width(), height() ), overlayColor );

    // 선택 영역 표시
    if( isSelecting_ )
    {
        QRect rect( startPos_ * devicePixelRatio(), endPos_ * devicePixelRatio() );

        if( endPos_.x() < startPos_.x() ||
            endPos_.y() < startPos_.y() )
        {
            rect = QRect( endPos_ * devicePixelRatio(), startPos_ * devicePixelRatio() );
        }

        painter.scale( 1.0 / devicePixelRatio(), 1.0 / devicePixelRatio() );

        painter.setPen( QPen( Qt::red, 2 ) );
        painter.drawRect( rect );

        // 선택 영역만 원래 밝기로 표시
        painter.setCompositionMode( QPainter::CompositionMode_Clear );
        painter.fillRect( rect, Qt::transparent );
        painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
        painter.drawPixmap( rect, screenShot_, rect );
    }
}

void QSnippingWidget::keyPressEvent( QKeyEvent* event )
{
    if( event->key() == Qt::Key_Escape )
    {
        Q_EMIT sigUserCancelled();
        close();
    }

    QWidget::keyPressEvent( event );
}

void QSnippingWidget::mousePressEvent( QMouseEvent* event )
{
    if( event->button() == Qt::LeftButton )
    {
        setCursor( Qt::BlankCursor );
        isSelecting_ = true;
        startPos_ = event->pos();
        endPos_ = startPos_;
        update();
    }
}

void QSnippingWidget::mouseMoveEvent( QMouseEvent* event )
{
    if( isSelecting_ )
    {
        endPos_ = event->pos();
        update();
    }
}

void QSnippingWidget::mouseReleaseEvent( QMouseEvent* event )
{
    if( isSelecting_ && event->button() == Qt::LeftButton )
    {
        setCursor( Qt::CrossCursor );

        endPos_ = event->pos();
        isSelecting_ = false;

        QRect rect = QRect( startPos_ * devicePixelRatio(), endPos_ * devicePixelRatio() ).normalized();

        if( endPos_.x() < startPos_.x() ||
            endPos_.y() < startPos_.y() )
        {
            rect = QRect( endPos_ * devicePixelRatio(), startPos_ * devicePixelRatio() );
        }

        if( rect.width() > 0 && rect.height() > 0 )
        {
            selectedRegion_ = screenShot_.copy( rect );
            emit sigRegionSelected();
            close();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///

QSnippingTool::QSnippingTool( QWidget* Parent )
    : ElaWidget( Parent ), dwAffinity( 0 )
{
    setWindowTitle( tr("스니핑 도구" ) );
    setupUi();

    acSaveTo = new QAction( tr("저장"), this );
    acSaveTo->setShortcut( QKeySequence( "Ctrl+S" ) );
    connect( acSaveTo, &QAction::triggered, this, &QSnippingTool::saveScreenshot );
    addAction( acSaveTo );

    acCopyToClipboard = new QAction( tr("복사"), this );
    acCopyToClipboard->setShortcut( QKeySequence( "Ctrl+C" ) );
    connect( acCopyToClipboard, &QAction::triggered, this, &QSnippingTool::copyToClipboard );
    addAction( acCopyToClipboard );

}

QPushButton* QSnippingTool::GetSaveButton() const
{
    return btnSaveTo;
}

QPushButton* QSnippingTool::GetCopyButton() const
{
    return btnCopyToClipboard;
}

void QSnippingTool::SetDisplayAffinity( quint32 dwAffinity )
{
    this->dwAffinity = dwAffinity;
    for( auto w : vecSnippingWidget )
        w->SetDisplayAffinity( dwAffinity );
    SetWindowDisplayAffinity( (HWND)winId(), dwAffinity );
}

QPixmap QSnippingTool::RetrieveCaptureImage() const
{
    return screenshot;
}

void QSnippingTool::closeEvent( QCloseEvent* event )
{
    Q_EMIT sigCloseEvent( event );

    ElaWidget::closeEvent( event );
}

void QSnippingTool::keyPressEvent( QKeyEvent* event )
{
    if( event->key() == Qt::Key_Escape )
    {
        close();
    }
    ElaWidget::keyPressEvent( event );
}

void QSnippingTool::resizeEvent( QResizeEvent* event )
{
    if( screenshot.isNull() == false )
        lblCaptureImage->setPixmap( screenshot.scaled( lblCaptureImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );

    ElaWidget::resizeEvent( event );
}

void QSnippingTool::takeFullScreenshot()
{
    takeScreenshot( false, chkIncludeCursor->isChecked() );
}

void QSnippingTool::takeRegionScreenshot()
{
    takeScreenshot( true, chkIncludeCursor->isChecked() );
}

void QSnippingTool::takeDelayedScreenshot()
{
    int delay = cbxTimerInterval->currentData().toInt();
    QMessageBox::information( this, "지연 캡처", QString( "%1초 후 스크린샷이 촬영됩니다." ).arg( delay ) );

    this->hide();
    delayTimer->start( delay * 1000 );
}

void QSnippingTool::saveScreenshot()
{
    if( screenshot.isNull() )
    {
        QMessageBox::warning( this, tr("오류"), tr("저장할 스크린샷이 없습니다.") );
        return;
    }

    // 현재 날짜와 시간으로 기본 파일명 설정
    const QString defaultName = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh-mm-ss" ) + ".png";
    const QString filePath = QFileDialog::getSaveFileName( this, tr("스크린샷 저장"),
                                                           defaultName,
                                                           tr("PNG 파일 (*.png);;JPEG 파일 (*.jpg *.jpeg);;모든 파일 (*.*)"), nullptr, QFileDialog::ReadOnly );
    if( filePath.isEmpty() == true )
        return;

    do
    {
        bool IsAccepted = true; bool IsHandled = false;
        Q_EMIT sigSaveTo( filePath, &IsAccepted, &IsHandled );
        if( IsAccepted == false )
            break;

        QImageWriter Writer( filePath );
        bool IsSuccess = Writer.write( screenshot.toImage() );

        if( IsSuccess == true  )
        {
            if( IsHandled == true )
                break;

            QMessageBox::information( this, tr( "저장 완료" ), tr( "스크린샷이 성공적으로 저장되었습니다." ) );
            break;
        }

        IsHandled = false;
        Q_EMIT sigSaveFailed( filePath, Writer.error(), Writer.errorString(), &IsHandled );
        if( IsHandled == true )
            break;

        QMessageBox::critical( this, tr( "저장 실패" ), tr( "스크린샷을 저장하는 중 오류가 발생했습니다." ) );
        break;

    } while( false );
}

void QSnippingTool::copyToClipboard()
{
    if( screenshot.isNull() )
    {
        QMessageBox::warning( this, tr("오류"), tr("복사할 스크린샷이 없습니다.") );
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setPixmap( screenshot );
    QMessageBox::information( this, tr("복사 완료"), tr("스크린샷이 클립보드에 복사되었습니다.") );
}

void QSnippingTool::onRegionSelected()
{
    // 영역 선택 결과 수신
    QSnippingWidget* snipper = qobject_cast< QSnippingWidget* >( sender() );
    if( snipper )
    {
        screenshot = snipper->SelectedRegion();

        // 이미지 라벨에 표시
        lblCaptureImage->setPixmap( screenshot.scaled( lblCaptureImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );

        // 저장 및 복사 버튼 활성화
        btnSaveTo->setEnabled( true );
        btnCopyToClipboard->setEnabled( true );

        // 애플리케이션 창 다시 표시
        this->show();

        for( const auto w : vecSnippingWidget )
            w->deleteLater();
        vecSnippingWidget.clear();
    }
}

void QSnippingTool::takeScreenshotWithTimer()
{
    delayTimer->stop();
    takeScreenshot( false, chkIncludeCursor->isChecked() );
}

void QSnippingTool::setupUi()
{
    // 이미지 레이블 생성
    lblCaptureImage = new QLabel();
    lblCaptureImage->setAlignment( Qt::AlignCenter );
    lblCaptureImage->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    lblCaptureImage->setMinimumSize( 800, 600 );
    // 초기 메시지 표시
    lblCaptureImage->setText( tr("화면 캡처를 시작하려면 버튼을 누르세요.") );

    cbxTimerInterval = new QComboBox( this );
    cbxTimerInterval->addItem( tr("3초"), 3 );
    cbxTimerInterval->addItem( tr("5초"), 5 );
    cbxTimerInterval->addItem( tr("10초"), 10 );

    chkIncludeCursor = new QCheckBox( tr("마우스 포인터 포함"), this );

    btnFullCapture = new QPushButton( tr("전체 화면"), this );
    connect( btnFullCapture, &QPushButton::clicked, this, &QSnippingTool::takeFullScreenshot );

    btnRegionCapture = new QPushButton( tr("영역 지정"), this );
    connect( btnRegionCapture, &QPushButton::clicked, this, &QSnippingTool::takeRegionScreenshot );

    btnTimerCapture = new QPushButton( tr("지연 캡처"), this );
    connect( btnTimerCapture, &QPushButton::clicked, this, &QSnippingTool::takeDelayedScreenshot );

    btnSaveTo = new QPushButton( tr("저장"), this );
    connect( btnSaveTo, &QPushButton::clicked, this, &QSnippingTool::saveScreenshot );
    btnSaveTo->setEnabled( false );

    btnCopyToClipboard = new QPushButton( tr("복사"), this );
    connect( btnCopyToClipboard, &QPushButton::clicked, this, &QSnippingTool::copyToClipboard );
    btnCopyToClipboard->setEnabled( false );

    QHBoxLayout* delayLayout = new QHBoxLayout();
    delayLayout->addWidget( btnTimerCapture );
    delayLayout->addWidget( cbxTimerInterval );
    delayLayout->addWidget( chkIncludeCursor );

    buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget( btnFullCapture );
    buttonLayout->addWidget( btnRegionCapture );
    buttonLayout->addLayout( delayLayout );
    buttonLayout->addStretch();
    buttonLayout->addWidget( btnSaveTo );
    buttonLayout->addWidget( btnCopyToClipboard );

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget( lblCaptureImage );
    mainLayout->addLayout( buttonLayout );

    delayTimer = new QTimer( this );
    connect( delayTimer, &QTimer::timeout, this, &QSnippingTool::takeScreenshotWithTimer );
}

void QSnippingTool::takeScreenshot( bool region, bool includeMouse )
{
    // 메인 창 숨기기
    this->hide();

    // 스크린샷을 찍기 전에 잠시 대기 (UI가 사라지도록)
    QTimer::singleShot( 500, [this, region, includeMouse]() {
        bool IsCanceled = false;
        Q_EMIT sigCaptureStart( &IsCanceled );
        if( IsCanceled == true )
            return;

        if( region )
            takeScreenshotByRegion( includeMouse );
        else
            takeScreenshotByFull( includeMouse );

        Q_EMIT sigCaptureFinished();
    } );
}

void QSnippingTool::takeScreenshotByFull( bool IncludeMouse )
{
    nsDXGI::CDXGICapture DXGI;
    if( FAILED( DXGI.Initialize() ) )
        return;

    QRect FullRect;

    for( int idx = 0; idx < DXGI.GetDublicatorMonitorInfoCount(); ++idx )
    {
        const auto Info = DXGI.GetDublicatorMonitorInfo( idx );
        if( Info == nullptr )
            continue;

        FullRect |= QRect( Info->Bounds.X, Info->Bounds.Y, Info->Bounds.Width, Info->Bounds.Height );
    }

    QPixmap ScreenShot( FullRect.size() );
    ScreenShot.fill( Qt::transparent );
    QPainter Painter( &ScreenShot );

    for( auto scr : QGuiApplication::screens() )
    {
        const auto ni = scr->nativeInterface<QNativeInterface::QWindowsScreen>();
        if( ni == nullptr )
            continue;

        const auto mon = ni->handle(); int MonitorIdx = -1;

        for( int idx = 0; idx < DXGI.GetDublicatorMonitorInfoCount(); ++idx )
        {
            const auto Info = DXGI.GetDublicatorMonitorInfo( idx );
            if( Info == nullptr )
                continue;

            if( mon != Info->Handle )
                continue;

            MonitorIdx = Info->Idx;
            break;
        }

        const auto Info = DXGI.GetDublicatorMonitorInfo( MonitorIdx );
        nsDXGI::tagScreenCaptureFilterConfig config;
        config.MonitorIdx           = MonitorIdx;
        config.ShowCursor           = IncludeMouse ? TRUE : FALSE;
        config.RotationMode         = nsDXGI::tagFrameRotationMode_Auto;
        config.OutputSize.Width     = Info->Bounds.Width;
        config.OutputSize.Height    = Info->Bounds.Height;
        config.SizeMode             = nsDXGI::tagFrameSizeMode_AutoSize;
        DXGI.SetConfig( config );
        QPixmap fullScreenshot = DXGI.CaptureToPixmap( L".jpg", nullptr, nullptr );
        Painter.drawPixmap( Info->Bounds.X, Info->Bounds.Y, fullScreenshot );
    }

    screenshot = ScreenShot;

    // 화면에 표시
    lblCaptureImage->setPixmap( screenshot.scaled( lblCaptureImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );

    // 저장 및 복사 버튼 활성화
    btnSaveTo->setEnabled( true );
    btnCopyToClipboard->setEnabled( true );

    // 애플리케이션 창 다시 표시
    this->show();
}

void QSnippingTool::takeScreenshotByRegion( bool IncludeMouse )
{
    nsDXGI::CDXGICapture DXGI;
    if( FAILED( DXGI.Initialize() ) )
        return;

    for( auto scr : QGuiApplication::screens() )
    {
        const auto ni = scr->nativeInterface<QNativeInterface::QWindowsScreen>();
        if( ni == nullptr )
            continue;

        const auto mon = ni->handle(); int MonitorIdx = -1;

        for( int idx = 0; idx < DXGI.GetDublicatorMonitorInfoCount(); ++idx )
        {
            const auto Info = DXGI.GetDublicatorMonitorInfo( idx );
            if( Info == nullptr )
                continue;

            if( mon != Info->Handle )
                continue;

            MonitorIdx = Info->Idx;
            break;
        }

        if( MonitorIdx < 0 )
            continue;

        const auto Info = DXGI.GetDublicatorMonitorInfo( MonitorIdx );
        nsDXGI::tagScreenCaptureFilterConfig config;
        config.MonitorIdx           = MonitorIdx;
        config.ShowCursor           = IncludeMouse ? TRUE : FALSE;
        config.RotationMode         = nsDXGI::tagFrameRotationMode_Auto;
        config.OutputSize.Width     = Info->Bounds.Width;
        config.OutputSize.Height    = Info->Bounds.Height;
        config.SizeMode             = nsDXGI::tagFrameSizeMode_AutoSize;
        DXGI.SetConfig( config );
        QPixmap fullScreenshot = DXGI.CaptureToPixmap( L".jpg", nullptr, nullptr );

        // 영역 선택 위젯 표시
        QSnippingWidget* snipper = new QSnippingWidget( fullScreenshot );
        snipper->setScreen( scr );
        snipper->setGeometry( scr->geometry() );
        snipper->SetDisplayAffinity( dwAffinity );
        vecSnippingWidget.push_back( snipper );

        connect( snipper, &QSnippingWidget::sigRegionSelected, this, &QSnippingTool::onRegionSelected );
        connect( snipper, &QSnippingWidget::sigUserCancelled, [this]() {
            // 애플리케이션 창 다시 표시
            show();

            for( const auto w : vecSnippingWidget )
                w->deleteLater();
            vecSnippingWidget.clear();
        } );
    }
}
