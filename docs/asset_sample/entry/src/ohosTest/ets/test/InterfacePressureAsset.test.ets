import asset from '@ohos.security.asset';
import { sleep, stringToArray } from './common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { BusinessError } from '@ohos.base';

let promise_case_num = 1000
let promise_base_alias = '11111111'
let secret1 = '1111111111111111'
let secret2 = '2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222'
let promise_label1 = 'promise_label1'

// 跑一遍要14分钟，需要18个小时 计算下来大概77次够 所以跑这么多次
let total_cycle_times = 77

export default function interfacePressurePromiseTest() {
  describe('interfacePressurePromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('runAddTest', 0, async () => {
      for (let index = 0; index < promise_case_num; index++) {
        let alias = promise_base_alias.concat(index.toString())
        let attr: asset.AssetMap = new Map();
        attr.set(asset.Tag.ALIAS, stringToArray(alias))
        attr.set(asset.Tag.SECRET, stringToArray(secret1))
        let promise_label_tackled = promise_label1.concat(Math.floor(index / 100).toString())
        attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray(promise_label_tackled))
        attr.set(asset.Tag.DATA_LABEL_NORMAL_2, stringToArray("normal_label2"))
        attr.set(asset.Tag.DATA_LABEL_NORMAL_3, stringToArray("normal_label3"))
        attr.set(asset.Tag.DATA_LABEL_NORMAL_4, stringToArray("normal_label4"))
        attr.set(asset.Tag.DATA_LABEL_CRITICAL_1, stringToArray("critical_label1"))
        attr.set(asset.Tag.DATA_LABEL_CRITICAL_2, stringToArray("critical_label2"))
        attr.set(asset.Tag.DATA_LABEL_CRITICAL_3, stringToArray("critical_label3"))
        attr.set(asset.Tag.DATA_LABEL_CRITICAL_4, stringToArray("critical_label4"))
        attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
        attr.set(asset.Tag.IS_PERSISTENT, true)
        try {
          await asset.add(attr)
          expect(true).assertTrue()
        } catch (error) {
          expect(error.code).assertEqual(asset.ErrorCode.DUPLICATED)
        }
      }
    })
    it('runQueryTest', 0, async () => {
      for (let index = 0; index < promise_case_num; index++) {
        let alias = promise_base_alias.concat(index.toString())
        let attr: asset.AssetMap = new Map();
        attr.set(asset.Tag.ALIAS, stringToArray(alias))
        try {
          await asset.query(attr)
          expect(true).assertTrue()
        } catch (error) {
          expect(error.code).assertEqual(asset.ErrorCode.NOT_FOUND)
        }
      }
    })
    it('runUpdateTest', 0, async () => {
      let attr21: asset.AssetMap = new Map();
      attr21.set(asset.Tag.SECRET, stringToArray(secret2))
      let alias_list1 = new Array<String>();
      for (let index = 0; index < promise_case_num; index++) {
        let alias = promise_base_alias.concat(index.toString())
        alias_list1.push(alias)
        let attr: asset.AssetMap = new Map();
        attr.set(asset.Tag.ALIAS, stringToArray(alias))
        try {
          await asset.update(attr, attr21)
          expect(true).assertTrue()
        } catch (error) {
          expect(error.code).assertEqual(asset.ErrorCode.NOT_FOUND)
        }
      }
    })

    it('runWholeTest', 0, async () => {
      for (let index = 0; index < total_cycle_times; index++) {
        for (let index = 0; index < promise_case_num; index++) {
          let alias = promise_base_alias.concat(index.toString())
          let attr: asset.AssetMap = new Map();
          attr.set(asset.Tag.ALIAS, stringToArray(alias))
          attr.set(asset.Tag.SECRET, stringToArray(secret1))
          let promise_label_tackled = promise_label1.concat(Math.floor(index / 100).toString())
          attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray(promise_label_tackled))
          attr.set(asset.Tag.DATA_LABEL_NORMAL_2, stringToArray("normal_label2"))
          attr.set(asset.Tag.DATA_LABEL_NORMAL_3, stringToArray("normal_label3"))
          attr.set(asset.Tag.DATA_LABEL_NORMAL_4, stringToArray("normal_label4"))
          attr.set(asset.Tag.DATA_LABEL_CRITICAL_1, stringToArray("critical_label1"))
          attr.set(asset.Tag.DATA_LABEL_CRITICAL_2, stringToArray("critical_label2"))
          attr.set(asset.Tag.DATA_LABEL_CRITICAL_3, stringToArray("critical_label3"))
          attr.set(asset.Tag.DATA_LABEL_CRITICAL_4, stringToArray("critical_label4"))
          attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
          attr.set(asset.Tag.IS_PERSISTENT, true)
          try {
            await asset.add(attr)
            expect(true).assertTrue()
          } catch (error) {
            expect(error.code).assertEqual(asset.ErrorCode.DUPLICATED)
          }
        }

        for (let index = 0; index < promise_case_num; index++) {
          let alias = promise_base_alias.concat(index.toString())
          let attr: asset.AssetMap = new Map();
          attr.set(asset.Tag.ALIAS, stringToArray(alias))
          try {
            await asset.query(attr)
            expect(true).assertTrue()
          } catch (error) {
            expect(error.code).assertEqual(asset.ErrorCode.NOT_FOUND)
          }
        }

        for (let index = 0; index < promise_case_num; index++) {
          let attr: asset.AssetMap = new Map();
          let promise_label_tackled = promise_label1.concat(Math.floor(index / 100).toString())
          attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray(promise_label_tackled))
          try {
            await asset.query(attr)
            expect(true).assertTrue()
          } catch (error) {
            expect(error.code).assertEqual(asset.ErrorCode.NOT_FOUND)
          }
        }

        let attr21: asset.AssetMap = new Map();
        attr21.set(asset.Tag.SECRET, stringToArray(secret2))
        let alias_list1 = new Array<String>();
        for (let index = 0; index < promise_case_num; index++) {
          let alias = promise_base_alias.concat(index.toString())
          alias_list1.push(alias)
          let attr: asset.AssetMap = new Map();
          attr.set(asset.Tag.ALIAS, stringToArray(alias))
          try {
            await asset.update(attr, attr21)
            expect(true).assertTrue()
          } catch (error) {
            expect(error.code).assertEqual(asset.ErrorCode.NOT_FOUND)
          }
        }

        for (let index = 0; index < promise_case_num; index++) {
          let alias = promise_base_alias.concat(index.toString())
          let attr: asset.AssetMap = new Map();
          attr.set(asset.Tag.ALIAS, stringToArray(alias))
          try {
            await asset.remove(attr)
            expect(true).assertTrue()
          } catch (error) {
            expect(error.code).assertEqual(asset.ErrorCode.NOT_FOUND)
          }
        }
      }
    })
  })
}