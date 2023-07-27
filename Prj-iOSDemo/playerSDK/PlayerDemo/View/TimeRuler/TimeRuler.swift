//
//  TimeRuler.swift
//  PlayerDemo
//
//  Created by 秦骏 on 2022/1/27.
//

import Foundation
import UIKit

// 标记频率
enum RulerMarkFrequency: Int {
    case hour      = 3600 //小时标记频率3600秒
    case minute_30 = 1800  //20分钟标记频率
    case minute_20 = 1200  //20分钟标记频率
    case minute_10 = 600  //10分钟标记频率
    case minute_5 = 300  //5分钟标记频率
    case minute_1  = 60  //2分钟标记评率
    // 如需要更小时间在此添加枚举值
}

// 侧边多出部分宽度
let sideOffset: CGFloat = 30.0
//  底部部分高度
let bottomOffset: CGFloat = 20.0
//  刻度部分高度
let markOffset: CGFloat = 10.0
//  空隙
let spaceY: CGFloat = 5.0
// 时间尺最大宽度
let rulerMaxWidth: CGFloat = 24000.0

// 时间尺标记
class STimeRulerMark: NSObject {
    var frequency: RulerMarkFrequency? //标记频率
    var size: CGSize? // 标记尺寸
    var color: UIColor = UIColor.init(white: 0.43, alpha: 1.0) //标记颜色
    var font: UIFont = UIFont.systemFont(ofSize: 9.0)
    var textColor: UIColor = UIColor.init(white: 0.43, alpha: 1.0)
}

// 时间尺绘图
class STimeRulerLayer: CALayer {
    // 最小标记
    lazy var minorMark: STimeRulerMark = {
        let mark = STimeRulerMark.init()
        mark.frequency = RulerMarkFrequency.minute_1
        mark.size = CGSize.init(width: 1.0, height: 6.0)
        return mark
    }()
    // 中等标记
    lazy var middleMark: STimeRulerMark = {
        let mark = STimeRulerMark.init()
        mark.frequency = RulerMarkFrequency.minute_10
        mark.size = CGSize.init(width: 1.0, height: 8.0)
        return mark
    }()
    // 大标记
    lazy var majorMark: STimeRulerMark = {
        let mark = STimeRulerMark.init()
        mark.frequency = RulerMarkFrequency.hour
        mark.size = CGSize.init(width: 1.0, height: 10.0)
        return mark
    }()
    
    // 选中区域
    var selectedRange: [[String: Int]]?{
        didSet{
            setNeedsDisplay()
        }
    }
    
    override var frame: CGRect{
        didSet{
            // frame改变需要重绘
            super.frame = frame
            setNeedsDisplay()
        }
    }
    
    
    override init() {
        super.init()
        backgroundColor = UIColor.clear.cgColor
        
    }
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    
    // 重绘
    override func display() {
        CATransaction.begin()
        CATransaction.setDisableActions(true)
        drawToImage()
        CATransaction.commit()
    }
    
    private func drawToImage() -> Void{
        
        var frequency: RulerMarkFrequency = .minute_10
        // 每小时占用的宽度
        let hourWidth: CGFloat = (bounds.width - sideOffset * 2) / 24.0
        
        // 根据宽度来判断显示标记的级别
        if hourWidth / 60.0 >= 6.0 {
            frequency = .minute_1
        }
        else if hourWidth / 60.0 >= 5.0{
            frequency = .minute_5
        }
        else if hourWidth / 60.0 >= 4.0{
            frequency = .minute_10
        }
        else if hourWidth / 60.0 >= 1.8{
            frequency = .minute_10
        }
        else if hourWidth / 60.0 >= 0.8{
            frequency = .minute_30
        }
        else{
            frequency = .hour
        }
        let rectYlog: String = "textRectY："
        print(rectYlog,hourWidth)
        // 计算有多少个标记
        let numberOfLine: Int = 24 * 3600 / frequency.rawValue
        let lineOffset: CGFloat = (bounds.width - sideOffset * 2) / CGFloat(integerLiteral: numberOfLine)
    
        UIGraphicsBeginImageContextWithOptions(bounds.size, false, UIScreen.main.scale)
        let ctx = UIGraphicsGetCurrentContext()
        let attributeString = NSAttributedString.init(string: "00:00", attributes: [NSAttributedString.Key.font: UIFont.systemFont(ofSize: 11)])
        // 计算文字最宽宽度
        let hourTextWidth = attributeString.size().width
        
        // 绘制选中区域
        if let selectedRangeArray = selectedRange {
            // 显示区域颜色
            ctx?.setFillColor(UIColor.init(red: 238.0/255.0, green: 165.0/255, blue: 133.0/255, alpha: 1.0).cgColor)
            for selectedItem in selectedRangeArray {
                // 绘制选中区域
                let startSecond: Int = selectedItem["start"]!
                let endSecond:   Int = selectedItem["end"]!
                let x: CGFloat = (CGFloat(integerLiteral: startSecond) / (24 * 3600.0)) * (bounds.width - sideOffset * 2) + sideOffset
                let width: CGFloat = CGFloat(integerLiteral: (endSecond - startSecond)) / (24 * 3600.0) * (bounds.width - sideOffset * 2)
                let rect: CGRect = CGRect.init(x: x, y: 0, width: width, height: bounds.height)
                ctx?.fill(rect)
            }
        }
        
        for i in 0...numberOfLine {
            // 计算每个标记的属性
            let position: CGFloat = CGFloat(i) * lineOffset
            let timeSecond: Int = i * frequency.rawValue
            var showText: Bool = false
            var timeString: String = "00:00"
            var mark: STimeRulerMark = minorMark
            
            if timeSecond % 3600 == 0{
                // 小时标尺
                mark = majorMark
                if hourWidth > (hourTextWidth + 5.0){
                    // 每小时都能画时间
                    showText = true
                }
                else {
                    // 每两小时画一个时间
                    showText = timeSecond % (3600 * 2) == 0
                    if(showText){
                        mark = majorMark
                    }else{
                        mark = middleMark
                    }
                }
                
            }
            else if timeSecond % 1800 == 0{
                // 每30分钟的标尺
                mark = middleMark
                if((frequency == .minute_30)||(frequency == .minute_20)||(frequency == .minute_10)||(frequency == .minute_5)||(frequency == .minute_1)){
                   
                    if hourWidth/2 > (hourTextWidth + 5.0){
                        showText = true
                    }
                }else{
                    showText = false
                }
            }
            else if timeSecond % 1200 == 0{
                // 每20分钟的标尺
                mark = middleMark
                if((frequency == .minute_20)||(frequency == .minute_10)||(frequency == .minute_5)||(frequency == .minute_1)){
                    if hourWidth/6 > (hourTextWidth + 5.0){
                        showText = true
                    }

                }else{
                    showText = false
                }
            }
            else if timeSecond % 600 == 0{
                // 每10分钟的标尺
                mark = middleMark
                if hourWidth/6 > (hourTextWidth + 5.0){
                    showText = true
                }
            }
            else if timeSecond % 300 == 0{
                // 每5分钟的标尺
                mark = middleMark
                if((frequency == .minute_5)||(frequency == .minute_1)){
                    showText = true
                }else{
                    showText = false
                }
            }
            
            let hour: Int = timeSecond / 3600
            let min:  Int = timeSecond % 3600 / 60
            timeString = String(format: "%02d:%02d", hour, min)
            
            drawMarkIn(context: ctx, position: position, timeString: timeString, mark: mark, showText: showText)

        }
        
        let imageToDraw: UIImage = UIGraphicsGetImageFromCurrentImageContext()!
        UIGraphicsEndImageContext()
        contents = imageToDraw.cgImage
        
    }
    
    private func drawMarkIn(context: CGContext?, position: CGFloat, timeString: String, mark: STimeRulerMark, showText: Bool) -> Void {
        let textAttribute = [NSAttributedString.Key.font: mark.font,
                             NSAttributedString.Key.foregroundColor: mark.textColor]
        let attributeString = NSAttributedString.init(string: timeString, attributes: textAttribute)
        let textSize: CGSize = attributeString.size()
        let rectX: CGFloat = position + sideOffset - mark.size!.width * 0.5
        let btmRect: CGRect = CGRect.init(x: rectX, y: bounds.height - markOffset - bottomOffset , width: mark.size!.width, height: mark.size!.height)
        context?.setFillColor(mark.color.cgColor)
        context?.fill(btmRect)
        if showText {
            // 绘制时间文字
            let textRectX: CGFloat = position + sideOffset - textSize.width * 0.5
            let textRectY: CGFloat = bounds.height  - (bottomOffset - spaceY)
          
            let textRect: CGRect = CGRect.init(x: textRectX, y: textRectY, width: textSize.width, height: bottomOffset - spaceY)
            let ocString: NSString = timeString as NSString
            ocString.draw(in: textRect, withAttributes: textAttribute)
        }
    }
}


// 时间尺
class TimeRuler: UIControl {
    var currentTime: Int = 0
    
    var rulerLayer: STimeRulerLayer?
    var rulerWidth: CGFloat = 0.0
    var scrollView: UIScrollView?
    var topLine: UIView?
    var btmLine: UIView?
    // 缩放时初始比例
    var startScale: CGFloat?
    
    // 判断是否是手动选择的时间
    var isTouch: Bool = false
    
    // 缩放前尺子宽度
    var oldRulerWidth: CGFloat = 0;
    
    func setSelectedRange(rangeArray: [[String:Int]]) -> Void {
        rulerLayer?.selectedRange = rangeArray
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        defaultValue()
        setupUI()
    }
    
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        defaultValue()
        fatalError("init(coder:) has not been implemented")
    }
    
    override func awakeFromNib() {
        super.awakeFromNib()
        setupUI()
    }
    
    private func defaultValue() -> Void {
        rulerWidth = 10.0 * bounds.width
        
    }
    
    
    private func setupUI() -> Void {
        setupLineUI()
        setupScrollViewUI()
        setupRulerLayer()
        
        let pinch: UIPinchGestureRecognizer = UIPinchGestureRecognizer.init(target: self, action: #selector(pinchAction(recoginer:)))
        addGestureRecognizer(pinch)
    }
    
    @objc private func pinchAction(recoginer: UIPinchGestureRecognizer) -> Void{
        if recoginer.state == .began{
            startScale = recoginer.scale
        }
        else if recoginer.state == .changed{
            // 缩放时更新layerFrame
            updateFrame(scale: recoginer.scale / startScale!)
            startScale = recoginer.scale
        }
        
    }
    
    private func updateFrame(scale: CGFloat) -> Void {
        var updateRulerWidth: CGFloat = rulerLayer!.bounds.width * scale
        if updateRulerWidth < (bounds.width + 2 * sideOffset){
            updateRulerWidth = (bounds.width + 2 * sideOffset)
        }
        
        if updateRulerWidth > rulerMaxWidth{
            updateRulerWidth = rulerMaxWidth
        }
        
        oldRulerWidth = rulerWidth
        rulerWidth = updateRulerWidth
        setNeedsLayout()
    }
    
    private func setupLineUI() -> Void {
        topLine = UIView.init()
        btmLine = UIView.init()
        topLine?.backgroundColor = UIColor.init(white: 0.43, alpha: 1.0)
        btmLine?.backgroundColor = UIColor.init(white: 0.43, alpha: 1.0)
        addSubview(topLine!)
        addSubview(btmLine!)
    }
    
    private func setupScrollViewUI() -> Void{
        scrollView = UIScrollView.init(frame: bounds)
        scrollView?.delegate = self;
        scrollView?.pinchGestureRecognizer?.isEnabled = false
        scrollView?.showsHorizontalScrollIndicator = false
        scrollView?.bounces = false
       // scrollView?.contentInsetAdjustmentBehavior = .never
        addSubview(scrollView!)
    }
    
    private func setupRulerLayer() -> Void{
        rulerLayer = STimeRulerLayer.init()
        scrollView?.layer.addSublayer(rulerLayer!)
    }
    
    
    override func layoutSubviews() {
        super.layoutSubviews()
        
        let sideInset = bounds.width / 2.0
        scrollView?.frame = bounds
        scrollView?.contentInset = UIEdgeInsets.init(top: 0, left: sideInset - sideOffset, bottom: 0, right: sideInset - sideOffset)
        
        CATransaction.begin()
        CATransaction.setDisableActions(true)
        rulerLayer?.frame = CGRect.init(x: 0, y: 0, width: rulerWidth, height: bounds.height)
        CATransaction.commit()
        
        scrollView?.contentSize = CGSize.init(width: rulerWidth, height: bounds.height)
        
        topLine?.frame = CGRect.init(x: 0, y: 0, width: bounds.width, height: 1.0)
        btmLine?.frame = CGRect.init(x: 0, y: bounds.height - bottomOffset-markOffset, width: bounds.width, height: 1.0)
        // 保证缩放过程中心保持不变
        scrollView?.contentOffset = contentOffset(current: currentTime)
        
    }
    
    // 根据当前选中的值, 计算contentOffset
    private func contentOffset(current: Int) -> CGPoint{
        let proportion: CGFloat = CGFloat(integerLiteral: current) / (24 * 3600.0)
        let proportionWidth: CGFloat = (scrollView!.contentSize.width - sideOffset * 2) * proportion
        return CGPoint.init(x: proportionWidth - scrollView!.contentInset.left, y: scrollView!.contentOffset.y)
    }
    

}

extension TimeRuler: UIScrollViewDelegate{
    func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
        isTouch = true
    }
    
    func scrollViewDidScroll(_ scrollView: UIScrollView) {
        let proportionWidth: CGFloat = scrollView.contentOffset.x + scrollView.contentInset.left
        let proportion: CGFloat = proportionWidth / (scrollView.contentSize.width - sideOffset * 2)
        let value: Int = Int(exactly: ceil(proportion * 24 * 3600))!
        currentTime = value
    }
    
    func scrollViewDidEndDecelerating(_ scrollView: UIScrollView) {
        isTouch = false
    }
    
    func scrollViewDidEndDragging(_ scrollView: UIScrollView, willDecelerate decelerate: Bool) {
        if (decelerate) {
            //禁止惯性移动
            DispatchQueue.global(qos: .background).async {
                DispatchQueue.main.async {
                    scrollView.setContentOffset(scrollView.contentOffset, animated: false)
                }
            }
        }
        if !decelerate {
            isTouch = false
        }
    }
    
}
