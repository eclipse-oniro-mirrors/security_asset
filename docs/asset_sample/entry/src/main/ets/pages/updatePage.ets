import router from '@ohos.router'
import functions from '../common/constants/functions'
import { updateAssetPromise } from "./AssetCommon"

@Entry
@Component
struct updatePage {
  @State params: Record<string, string> = router.getParams() as Record<string, string>;
  @State User: string = this.params.name as string;
  @State pwd: string = this.params.pwd as string;
  @State label: string = this.params.label as string;
  @State ReservePassword: string = this.pwd;
  @State ReserveLabel: string = this.label;
  @State eventType: string = ''

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
        Text("更新")
          .fontFamily("HarmonyHeiTi")
          .fontSize(20)
          .fontColor(0x182431)
          .lineHeight(28)
          .fontWeight(700)
          .margin({ left: '5%' })
          .textAlign(TextAlign.Center)
      }.width('86.7%').height(56)

      Column() {
        /* 账号 */
        Row() {
          Text("账号")
            .fontFamily("HarmonyHeiTi")
            .fontSize(16)
            .fontColor(0x182431)
            .lineHeight(22)
            .fontWeight(500)
            .textAlign(TextAlign.Start)
            .padding({ top: '2.3%', bottom: '1.1%' })
        }
        .width('86.7%').height('6.7%')

        /* 用户名输入 */
        Row() {
          Text(this.User)
            .fontFamily("HarmonyHeiTi")
            .fontSize(16)
            .opacity(0.6)
            .fontColor(0x182431)
            .lineHeight(22)
            .fontWeight(400)
            .textAlign(TextAlign.Start)
            .padding({ top: '2.3%', bottom: '1.1%' })
        }
        .width('86.7%').height('6.7%')

        Line()
          .width('86.7%')
          .height('0.5vp')
          .margin({
            left: '12vp',
            right: '12vp'
          })
          .backgroundColor('#0D182431')

        /* 密码 */
        Row() {
          Text("*").fontColor(0xFFE84026)
          Text("密码/信用卡号")
            .fontFamily("HarmonyHeiTi")
            .fontSize(16)
            .fontColor(0x182431)
            .lineHeight(22)
            .fontWeight(500)
            .padding({ top: '2.3%', bottom: '1.1%' })
        }
        .width('86.7%').height('6.7%')

        /* 密码输入 */
        Row() {
          TextInput({ text: this.pwd })
            .type(InputType.Password)
            .defaultFocus(false)
            .fontColor(0x182431)
            .padding({ top: '1.70%', right: '48.00px', bottom: '1.70%', left: '0' })
            .backgroundColor(Color.Transparent)
            .placeholderFont({ size: 16, weight: 400, family: 'HarmonyHeiTi', style: FontStyle.Normal })
            .onChange((value: string) => {
              this.ReservePassword = value;
            })
            .onTouch((event: TouchEvent) => {
              if (event.type === TouchType.Down) {
                this.eventType = 'Down9';
              }
            })
            .margin({ right: -12 })
        }
        .width('86.7%').height('6.7%')

        if (this.eventType === 'Down9') {
          Line()
            .width('86.7%')
            .height('0.5vp')
            .margin({
              left: '12vp',
              right: '12vp'
            })
            .backgroundColor(0x182431)
        } else {
          Line()
            .width('86.7%')
            .height('0.5vp')
            .margin({
              left: '12vp',
              right: '12vp'
            })
            .backgroundColor('#0D182431')
        }

        /* 标签 */
        Row() {
          Text("标签（最多4个，以\";\"分隔）")
            .fontFamily("HarmonyHeiTi")
            .fontSize(16)
            .fontColor(0x182431)
            .lineHeight(22)
            .fontWeight(500)
            .baselineOffset(0)
            .textAlign(TextAlign.Start)
            .padding({ top: '2.3%', bottom: '1.1%' })
        }
        .width('86.7%').height('6.7%')

        /* 标签输入 */
        Row() {
          TextInput({ text: this.label })
            .defaultFocus(false)
            .fontColor(0x182431)
            .padding({ top: '1.70%', right: '48.00px', bottom: '1.70%', left: '0' })
            .backgroundColor(Color.Transparent)
            .placeholderFont({ size: 16, weight: 400, family: 'HarmonyHeiTi', style: FontStyle.Normal })
            .onChange((value: string) => {
              this.ReserveLabel = value;
            })
            .onTouch((event: TouchEvent) => {
              if (event.type === TouchType.Down) {
                this.eventType = 'Down10';
              }
            })
        }
        .width('86.7%').height('6.7%')

        if (this.eventType === 'Down10') {
          Line()
            .width('86.7%')
            .height('0.5vp')
            .margin({
              left: '12vp',
              right: '12vp'
            })
            .backgroundColor(0x182431).margin({ bottom: 4 })
        } else {
          Line()
            .width('86.7%')
            .height('0.5vp')
            .margin({
              left: '12vp',
              right: '12vp'
            })
            .backgroundColor('#0D182431').margin({ bottom: 4 })
        }
      }
      .backgroundColor(0xFFFFFF).borderRadius(16)

      Blank()
      Row() {
        /* 底部按钮 */
        Button('更新', { type: ButtonType.Capsule, stateEffect: false })
          .backgroundColor(0x007DFF).fontColor('white')
          .borderRadius(20)
          .width('86.7%')
          .height(40)
          .enabled(functions.isUpdateClickable(this.ReservePassword))
          .onClick(() => {
            updateAssetPromise(this.User, this.ReservePassword, this.ReserveLabel)
          })
      }
      .margin({ bottom: '8.1%' })
    }
    // .margin({ right: 12, left: 12 })
    .width("100%").height('100%').backgroundColor(0xf1f3f5)
  }
}