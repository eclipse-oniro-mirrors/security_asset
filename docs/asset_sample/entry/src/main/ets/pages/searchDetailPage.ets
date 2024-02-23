import router from '@ohos.router';

interface AssetItem {
  alias: string;
  secret: string;
  dataLabel: string;
}

@Entry
@Component
struct MyTest {
  @State param: Record<string, Array<AssetItem>> = router.getParams() as Record<string, Array<AssetItem>>;
  @State data: Array<AssetItem> = this.param.data_array_map as Array<AssetItem>;

  build() {
    Column() {
      Row() {
        Image($r('app.media.back'))
          .height(18)
          .width(20)
          .onClick(() => {
            // 返回到上一页面，相当于pop操作
            router.back();
          })

        Text("查询结果")
          .fontFamily("HarmonyHeiTi")
          .fontSize(20)
          .fontColor(0x182431)
          .lineHeight(28)
          .fontWeight(700)
          .margin({ left: '5%' })
          .textAlign(TextAlign.Center)
      }
      .width('86.7%').height(56)

      Column() {
        List({ space: 0, initialIndex: 0 }) {
          ForEach(this.data, (item: AssetItem) => {
            ListItem() {
              Column() {
                Row() {
                  Row() {
                    Text("账号")
                      .fontFamily("HarmonyHeiTi")
                      .fontSize(16)
                      .fontColor(0x182431)
                      .lineHeight(22)
                      .fontWeight(500)
                      .margin({ top: '2.3%', bottom: '1.1%' })
                  }
                  Blank()
                  Row() {
                    Text(item.alias.toString())
                      .opacity(0.6)
                      .fontFamily("HarmonyHeiTi")
                      .fontSize(16)
                      .fontColor(0x182431)
                      .lineHeight(19)
                      .fontWeight(400)
                      .baselineOffset(0)
                      .margin({ top: '2.3%', bottom: '1.1%' })
                  }
                }
                .width('86.7%').height('7.14%')

                Line()
                  .width('86.7%')
                  .height('0.5vp')
                  .margin({
                    left: '12vp',
                    right: '12vp'
                  })
                  .backgroundColor('#0D182431')

                /* 密码/信用卡号 */
                if (item.secret) {
                  Row() {
                    Row() {
                      Text("密码/信用卡号")
                        .fontFamily("HarmonyHeiTi")
                        .fontSize(16)
                        .fontColor(0x182431)
                        .lineHeight(22)
                        .fontWeight(500)
                        .textAlign(TextAlign.Start)
                        .padding({ top: '2.3%', bottom: '1.1%' })
                    }

                    Blank()
                    Row() {
                      Text(item.secret.toString())
                        .opacity(0.6)
                        .fontFamily("HarmonyHeiTi")
                        .fontSize(16)
                        .fontColor(0x182431)
                        .lineHeight(19)
                        .fontWeight(400)
                        .textAlign(TextAlign.End)
                        .padding({ top: '2.3%', bottom: '1.1%' })
                    }
                  }.width('86.7%').height('6.7%')
                }
                Line()
                  .width('86.7%')
                  .height('0.5vp')
                  .margin({
                    left: '12vp',
                    right: '12vp'
                  })
                  .backgroundColor('#0D182431')
                /* 标签 */
                Row() {
                  Row() {
                    Text("标签")
                      .fontFamily("HarmonyHeiTi")
                      .fontSize(16)
                      .fontColor(0x182431)
                      .lineHeight(22)
                      .fontWeight(500)
                      .baselineOffset(0)
                      .textAlign(TextAlign.Start)
                      .padding({ top: '2.3%', bottom: '1.1%' })
                  }

                  Blank()
                  Row() {
                    Text(item.dataLabel.toString())
                      .opacity(0.6)
                      .fontFamily("HarmonyHeiTi")
                      .fontSize(16)
                      .fontColor(0x182431)
                      .lineHeight(19)
                      .fontWeight(400)
                      .baselineOffset(0)
                      .textAlign(TextAlign.End)
                      .padding({ top: '2.3%', bottom: '1.1%' })
                  }
                }.width('86.7%').height('7.14%')
              }
              .margin({ left: 12, right: -12, bottom: 12 })
              .backgroundColor(0xFFFFFF).borderRadius(16)
            }
          }, (item: AssetItem) => JSON.stringify(item))
        }
        .listDirection(Axis.Vertical) // 排列方向
        .edgeEffect(EdgeEffect.Spring) // 滑动到边缘无效果
      }
    }
    .width("100%").height("100%").backgroundColor(0xf1f3f5)
  }

  pageTransition() {
    // 定义页面进入时的效果，从右侧滑入，时长为1000ms，页面栈发生push操作时该效果才生效
    PageTransitionEnter({ type: RouteType.Push, duration: 300 })
      .slide(SlideEffect.Right)
    // 定义页面进入时的效果，从左侧滑入，时长为1000ms，页面栈发生pop操作时该效果才生效
    PageTransitionEnter({ type: RouteType.Pop, duration: 300 })
      .slide(SlideEffect.Left)
    // 定义页面退出时的效果，向左侧滑出，时长为1000ms，页面栈发生push操作时该效果才生效
    PageTransitionExit({ type: RouteType.Push, duration: 300 })
      .slide(SlideEffect.Left)
    // 定义页面退出时的效果，向右侧滑出，时长为1000ms，页面栈发生pop操作时该效果才生效
    PageTransitionExit({ type: RouteType.Pop, duration: 300 })
      .slide(SlideEffect.Right)
  }
}