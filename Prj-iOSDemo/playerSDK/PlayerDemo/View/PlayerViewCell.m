//
//  PlayerViewCell.m
//  PlayerDemo
//
//  Created by 秦骏 on 2021/12/18.
//

#import "PlayerViewCell.h"
#import "openGL20.h"
#import "Masonry.h"
@interface PlayerViewCell()
@property (nonatomic, strong) OpenGLView20* playerView;
@end
@implementation PlayerViewCell
-(instancetype)initWithFrame:(CGRect)frame{
    if (self = [super initWithFrame:frame]) {
       
        [self initUIView];
    }
    return self;
}
-(void)initUIView{
   
}
-(void)view:(OpenGLView20*)view{
    self.playerView = view;
    [self addSubview:self.playerView];
    [self.playerView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.mas_bottom).offset(-2);
        make.right.equalTo(self.mas_right).offset(-2);
        make.left.equalTo(self.mas_left).offset(2);
        make.top.equalTo(self.mas_top).offset(2);
    }];
}
-(OpenGLView20*)playerView{
    if (!_playerView) {
        _playerView = [[OpenGLView20 alloc]init];
    }
    return _playerView;
}
@end
